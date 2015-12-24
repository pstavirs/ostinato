#! /usr/bin/env python

# standard modules
import logging
import os
import re
import subprocess
import sys
import time

import pytest

from fabric.api import run, env, sudo
#from harness import Test, TestSuite, TestPreRequisiteError

sys.path.insert(1, '../binding')
from core import ost_pb, emul, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac, Mac
from protocols.ip4_pb2 import ip4, Ip4
from protocols.vlan_pb2 import vlan

use_defaults = True

if sys.platform == 'win32':
    tshark = r'C:\Program Files\Wireshark\tshark.exe'
else:
    tshark = 'tshark'

# initialize defaults - drone
host_name = '127.0.0.1'

# initialize defaults - DUT
env.use_shell = False
env.user = 'tc'
env.password = 'tc'
env.host_string = 'localhost:50022'

# setup logging
log = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

# command-line option/arg processing
if len(sys.argv) > 1:
    if sys.argv[1] in ('-d', '--use-defaults'):
        use_defaults = True
    if sys.argv[1] in ('-h', '--help'):
        print('%s [OPTION]...' % (sys.argv[0]))
        print('Options:')
        print(' -d --use-defaults   run using default values')
        print(' -h --help           show this help')
        sys.exit(0)

print('This module uses the following topology -')
print('')
print(' +-------+           +-------+')
print(' |       |Tx--->---Rx|-+     |')
print(' | Drone |           | v DUT |')
print(' |       |Rx---<---Tx|-+     |')
print(' +-------+           +-------+')
print('')
print('Drone has 2 ports connected to DUT. Packets sent on the Tx port')
print('are expected to be forwarded by the DUT and received back on the')
print('Rx port')
print('')

if not use_defaults:
    s = raw_input('Drone\'s Hostname/IP [%s]: ' % (host_name))
    host_name = s or host_name
    s = raw_input('DUT\'s Hostname/IP [%s]: ' % (env.host_string))
    env.host_string = s or env.host_string
    # FIXME: get inputs for dut rx/tx ports

# ================================================================= #
# ----------------------------------------------------------------- #
#                            FIXTURES
# ----------------------------------------------------------------- #
# ================================================================= #
# ----------------------------------------------------------------- #
# Baseline Configuration for subsequent testcases
# NOTES
#  * All test cases will emulate devices on both rx and tx ports
#  * Each test case will create traffic streams corresponding to
#    the devices to check
# ----------------------------------------------------------------- #

@pytest.fixture(scope='module')
def drone(request):
    drn = DroneProxy(host_name)

    log.info('connecting to drone(%s:%d)' % (drn.hostName(), drn.portNumber()))
    drn.connect()

    def fin():
        drn.disconnect()
    request.addfinalizer(fin)

    return drn

@pytest.fixture(scope='module')
def ports(request, drone):
    port_id_list = drone.getPortIdList()
    port_config_list = drone.getPortConfig(port_id_list)
    assert len(port_config_list.port) != 0

    # print port list and find default tx/rx ports
    tx_number = -1
    rx_number = -1
    print port_config_list
    print('Port List')
    print('---------')
    for port in port_config_list.port:
        print('%d.%s (%s)' % (port.port_id.id, port.name, port.description))
        # use a vhost port as default tx/rx port
        if ('vhost' in port.name or 'sun' in port.description.lower()):
            if tx_number < 0:
                tx_number = port.port_id.id
            elif rx_number < 0:
                rx_number = port.port_id.id
        if ('eth1' in port.name):
            tx_number = port.port_id.id
        if ('eth2' in port.name):
            rx_number = port.port_id.id

    assert tx_number >= 0
    assert rx_number >= 0

    print('Using port %d as tx port(s)' % tx_number)
    print('Using port %d as rx port(s)' % rx_number)

    ports.tx = ost_pb.PortIdList()
    ports.tx.port_id.add().id = tx_number;

    ports.rx = ost_pb.PortIdList()
    ports.rx.port_id.add().id = rx_number;
    return ports

@pytest.fixture(scope='module')
def dut(request):
    # Enable IP forwarding on the DUT (aka make it a router)
    sudo('sysctl -w net.ipv4.ip_forward=1')

@pytest.fixture(scope='module')
def dut_ports(request):
    dut_ports.rx = 'eth1'
    dut_ports.tx = 'eth2'

    # delete all configuration on the DUT interfaces
    sudo('ip address flush dev ' + dut_ports.rx)
    sudo('ip address flush dev ' + dut_ports.tx)
    return dut_ports

@pytest.fixture(scope='module')
def emul_ports(request, drone, ports):
    emul_ports = ost_pb.PortIdList()
    emul_ports.port_id.add().id = ports.tx.port_id[0].id;
    emul_ports.port_id.add().id = ports.rx.port_id[0].id;
    return emul_ports

@pytest.fixture(scope='module')
def dgid_list(request, drone, ports):
    # ----------------------------------------------------------------- #
    # create emulated device(s) on tx/rx ports - each test case will
    # modify and reuse these devices as per its needs
    # ----------------------------------------------------------------- #

    # delete existing devices, if any, on tx port
    dgid_list.tx = drone.getDeviceGroupIdList(ports.tx.port_id[0])
    drone.deleteDeviceGroup(dgid_list.tx)

    # add a emulated device group on tx port
    dgid_list.tx = ost_pb.DeviceGroupIdList()
    dgid_list.tx.port_id.CopyFrom(ports.tx.port_id[0])
    dgid_list.tx.device_group_id.add().id = 1
    log.info('adding tx device_group %d' % dgid_list.tx.device_group_id[0].id)
    drone.addDeviceGroup(dgid_list.tx)

    # delete existing devices, if any, on rx port
    dgid_list.rx = drone.getDeviceGroupIdList(ports.rx.port_id[0])
    drone.deleteDeviceGroup(dgid_list.rx)

    # add a emulated device group on rx port
    dgid_list.rx = ost_pb.DeviceGroupIdList()
    dgid_list.rx.port_id.CopyFrom(ports.rx.port_id[0])
    dgid_list.rx.device_group_id.add().id = 1
    log.info('adding rx device_group %d' % dgid_list.rx.device_group_id[0].id)
    drone.addDeviceGroup(dgid_list.rx)

    def fin():
        dgid_list = drone.getDeviceGroupIdList(ports.tx.port_id[0])
        drone.deleteDeviceGroup(dgid_list)
        dgid_list = drone.getDeviceGroupIdList(ports.rx.port_id[0])
        drone.deleteDeviceGroup(dgid_list)
    request.addfinalizer(fin)

    return dgid_list

@pytest.fixture(scope='module')
def stream_id(request, drone, ports):
    # ----------------------------------------------------------------- #
    # create stream on tx port - each test case will modify and reuse
    # this stream as per its needs
    # ----------------------------------------------------------------- #

    # delete existing streams, if any, on tx port
    sid_list = drone.getStreamIdList(ports.tx.port_id[0])
    drone.deleteStream(sid_list)

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.tx.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    def fin():
        drone.deleteStream(stream_id)
    request.addfinalizer(fin)

    return stream_id

@pytest.fixture
def dut_ip(request, dut_ports):
    sudo('ip address add 10.10.1.1/24 dev ' + dut_ports.rx)
    sudo('ip address add 10.10.2.1/24 dev ' + dut_ports.tx)

    def fin():
        sudo('ip address delete 10.10.1.1/24 dev ' + dut_ports.rx)
        sudo('ip address delete 10.10.2.1/24 dev ' + dut_ports.tx)
    request.addfinalizer(fin)

@pytest.fixture
def dut_vlans(request, dut_ports):
    class Devices(object):
        pass

    def create_vdev(devices, vlancfgs):
        # device(s) with ALL vlan tags are what we configure and add to netns;
        # those with intermediate no of vlans are created but not configured
        if len(vlancfgs) == 0:
            assert len(devices.rx) == len(devices.tx)
            dut_vlans.vlans = []
            for i in range(len(devices.rx)):
                vrf = 'vrf' + str(i+1)
                sudo('ip netns add ' + vrf)

                dev = devices.rx[i]
                sudo('ip link set ' + dev
                        + ' netns ' + vrf)
                sudo('ip netns exec ' + vrf
                        + ' ip addr add 10.1.1.1/24'
                        + ' dev ' + dev)
                sudo('ip netns exec ' + vrf
                        + ' ip link set ' + dev + ' up')

                dev = devices.tx[i]
                sudo('ip link set ' + dev
                        + ' netns ' + vrf)
                sudo('ip netns exec ' + vrf
                        + ' ip addr add 10.1.2.1/24'
                        + ' dev ' + dev)
                sudo('ip netns exec ' + vrf
                        + ' ip link set ' + dev + ' up')

                dut_vlans.vlans.append(dev[dev.find('.')+1:])
            return dut_vlans.vlans
        vcfg  = vlancfgs[0]
        new_devs = Devices()
        new_devs.tx = []
        new_devs.rx = []
        for dev in devices.rx+devices.tx:
            for k in range(vcfg['count']):
                vlan_id = vcfg['base'] + k*vcfg.get('step', 1)
                if 'tpid' in vcfg and vcfg['tpid'] == 0x88a8:
                    tpid = '802.1ad'
                else:
                    tpid = '802.1q'
                dev_name = dev + '.' + str(vlan_id)
                sudo('ip link add link ' + dev
                     + ' name ' + dev_name
                     + ' type vlan id ' + str(vlan_id)
                     + ' proto ' + tpid)
                sudo('ip link set ' + dev_name + ' up')
                if dev in devices.rx:
                    new_devs.rx.append(dev_name)
                else:
                    new_devs.tx.append(dev_name)
        print (str(len(devices.rx)*2*vcfg['count'])+' devices created')

        create_vdev(new_devs, vlancfgs[1:])

    def delete_vdev():
        if len(vlancfgs) == 0:
            return

        vrf_count = 1
        for vcfg in vlancfgs:
            vrf_count = vrf_count * vcfg['count']

        # deleting netns will also delete the devices inside it
        for i in range(vrf_count):
            vrf = 'vrf' + str(i+1)
            sudo('ip netns delete ' + vrf)

        # if we have only single tagged vlans, then we are done
        if len(vlancfgs) == 1:
            return

        # if we have 2 or more stacked vlan tags, we need to delete the
        # intermediate stacked vlan devices which are in the root namespace;
        # deleting first level tag vlan devices will delete vlan devices
        # stacked on top of it also
        vcfg  = vlancfgs[0]
        for dev in devices.tx+devices.rx:
            for k in range(vcfg['count']):
                vdev = dev + '.' + str(vcfg['base']+k)
                sudo('ip link delete ' + vdev)


    vlancfgs = getattr(request.function, 'vlan_cfg')
    devices = Devices()
    devices.tx = [dut_ports.tx]
    devices.rx = [dut_ports.rx]
    create_vdev(devices, vlancfgs)

    request.addfinalizer(delete_vdev)


# ================================================================= #
# ----------------------------------------------------------------- #
#                            TEST CASES
# ----------------------------------------------------------------- #
# ================================================================= #

@pytest.mark.parametrize('dev_cfg', [
    {'mac_step': 1, 'ip_step': 1},
    {'mac_step': 2, 'ip_step': 5},
])
def test_multiEmulDevNoVlan(drone, ports, dut, dut_ports, dut_ip,
        stream_id, emul_ports, dgid_list, dev_cfg):
    # ----------------------------------------------------------------- #
    # TESTCASE: Emulate multiple IPv4 devices (no vlans)
    #          DUT
    #        /.1   \.1
    #       /       \
    # 10.10.1/24  10.10.2/24
    #     /           \
    #    /.101-105     \.101-105
    #  Host1(s)      Host2(s)
    # ----------------------------------------------------------------- #

    num_devs = 5
    mac_step = dev_cfg['mac_step']
    ip_step = dev_cfg['ip_step']

    # configure the tx device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.tx.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.tx.device_group_id[0].id
    dg.core.name = "Host1"
    dg.device_count = num_devs
    dg.Extensions[emul.mac].address = 0x000102030a01
    if (mac_step != 1):
        dg.Extensions[emul.mac].step = mac_step
    ip = dg.Extensions[emul.ip4]
    ip.address = 0x0a0a0165
    ip.prefix_length = 24
    if (ip_step != 1):
        ip.step = ip_step
    ip.default_gateway = 0x0a0a0101

    drone.modifyDeviceGroup(devgrp_cfg)

    # configure the rx device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.rx.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.rx.device_group_id[0].id
    dg.core.name = "Host1"
    dg.device_count = num_devs
    dg.Extensions[emul.mac].address = 0x000102030b01
    if (mac_step != 1):
        dg.Extensions[emul.mac].step = mac_step
    ip = dg.Extensions[emul.ip4]
    ip.address = 0x0a0a0265
    ip.prefix_length = 24
    if (ip_step != 1):
        ip.step = ip_step
    ip.default_gateway = 0x0a0a0201

    drone.modifyDeviceGroup(devgrp_cfg)

    # configure the tx stream
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.tx.port_id[0])
    s = stream_cfg.stream.add()
    s.stream_id.id = stream_id.stream_id[0].id
    s.core.is_enabled = True
    s.control.packets_per_sec = 100
    s.control.num_packets = 10

    # setup stream protocols as mac:eth2:ip4:udp:payload
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac_mode = Mac.e_mm_resolve
    p.Extensions[mac].src_mac_mode = Mac.e_mm_resolve

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    ip = p.Extensions[ip4]
    ip.src_ip = 0x0a0a0165
    if ip_step == 1:
        ip.src_ip_mode = Ip4.e_im_inc_host
        ip.src_ip_count = num_devs
    else:
        vf = p.variable_field.add()
        vf.type = ost_pb.VariableField.kCounter32
        vf.offset = 12
        vf.mask  = 0x000000FF
        vf.value = 0x00000065
        vf.step  = ip_step
        vf.mode = ost_pb.VariableField.kIncrement
        vf.count = num_devs
    ip.dst_ip = 0x0a0a0265
    if ip_step == 1:
        ip.dst_ip_mode = Ip4.e_im_inc_host
        ip.dst_ip_count = num_devs
    else:
        vf = p.variable_field.add()
        vf.type = ost_pb.VariableField.kCounter32
        vf.offset = 16
        vf.mask  = 0x000000FF
        vf.value = 0x00000065
        vf.step  = ip_step
        vf.mode = ost_pb.VariableField.kIncrement
        vf.count = num_devs

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # FIXME(needed?): clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(ports.tx)
    drone.clearStats(ports.rx)

    # clear arp on DUT
    sudo('ip neigh flush all')
    arp_cache = run('ip neigh show')
    assert re.search('10.10.[1-2].1\d\d.*lladdr', arp_cache) == None

    # resolve ARP on tx/rx ports
    log.info('resolving Neighbors on tx/rx ports ...')
    drone.startCapture(emul_ports)
    drone.clearDeviceNeighbors(emul_ports)
    drone.resolveDeviceNeighbors(emul_ports)
    time.sleep(3)
    drone.stopCapture(emul_ports)

    # verify ARP Requests sent out from tx port
    buff = drone.getCaptureBuffer(emul_ports.port_id[0])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Tx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Tx capture buffer (filtered)')
    for i in range(num_devs):
        cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                    '-R', '(arp.opcode == 1)'
                      ' && (arp.src.proto_ipv4 == 10.10.1.'
                                                     +str(101+i*ip_step)+')'
                      ' && (arp.dst.proto_ipv4 == 10.10.1.1)'])
        print(cap_pkts)
        assert cap_pkts.count('\n') == 1

    # verify *no* ARP Requests sent out from rx port
    buff = drone.getCaptureBuffer(emul_ports.port_id[1])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Rx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Rx capture buffer (filtered)')
    for i in range(num_devs):
        cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                    '-R', '(arp.opcode == 1)'
                      ' && (arp.src.proto_ipv4 == 10.10.2.'
                                                     +str(101+i*ip_step)+')'
                      ' && (arp.dst.proto_ipv4 == 10.10.2.1)'])
        print(cap_pkts)
        assert cap_pkts.count('\n') == 0

    # retrieve and verify ARP Table on tx/rx ports
    log.info('retrieving ARP entries on tx port')
    device_list = drone.getDeviceList(emul_ports.port_id[0])
    device_config = device_list.Extensions[emul.port_device]
    neigh_list = drone.getDeviceNeighbors(emul_ports.port_id[0])
    devices = neigh_list.Extensions[emul.devices]
    log.info('ARP Table on tx port')
    for dev_cfg, device in zip(device_config, devices):
        resolved = False
        for arp in device.arp:
            # TODO: pretty print ip and mac
            print('%08x: %08x %012x' %
                    (dev_cfg.ip4, arp.ip4, arp.mac))
            if (arp.ip4 == dev_cfg.ip4_default_gateway) and (arp.mac):
                resolved = True
        assert resolved

    log.info('retrieving ARP entries on rx port')
    device_list = drone.getDeviceList(emul_ports.port_id[0])
    device_config = device_list.Extensions[emul.port_device]
    neigh_list = drone.getDeviceNeighbors(emul_ports.port_id[1])
    devices = neigh_list.Extensions[emul.devices]
    log.info('ARP Table on rx port')
    for dev_cfg, device in zip(device_config, devices):
        # verify *no* ARPs learnt on rx port
        assert len(device.arp) == 0
        for arp in device.arp:
            # TODO: pretty print ip and mac
            print('%08x: %08x %012x' %
                    (dev_cfg.ip4, arp.ip4, arp.mac))

    # ping the tx devices from the DUT
    for i in range(num_devs):
        out = run('ping -c3 10.10.1.'+str(101+i*ip_step), warn_only=True)
        assert '100% packet loss' not in out

    # ping the tx devices from the DUT
    for i in range(num_devs):
        out = run('ping -c3 10.10.2.'+str(101+i*ip_step), warn_only=True)
        assert '100% packet loss' not in out

    # We are all set now - so transmit the stream now
    drone.startCapture(ports.rx)
    drone.startTransmit(ports.tx)
    log.info('waiting for transmit to finish ...')
    time.sleep(3)
    drone.stopTransmit(ports.tx)
    drone.stopCapture(ports.rx)

    buff = drone.getCaptureBuffer(ports.rx.port_id[0])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Rx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Rx capture buffer (filtered)')
    for i in range(num_devs):
        cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                    '-R', '(ip.src == 10.10.1.' + str(101+i*ip_step) + ') '
                      ' && (ip.dst == 10.10.2.' + str(101+i*ip_step) + ')'
                      ' && (eth.dst == 00:01:02:03:0b:'
                                + format(1+i*mac_step, '02x')+')'])
        print(cap_pkts)
        assert cap_pkts.count('\n') == s.control.num_packets/num_devs
    os.remove('capture.pcap')

    drone.stopTransmit(ports.tx)
    run('ip neigh show')

@pytest.mark.parametrize('vlan_cfg', [
    [{'base': 11, 'count': 5}],

    [{'base': 11, 'count': 2},
     {'base': 21, 'count': 3}],

    [{'base': 11, 'count': 2, 'tpid': 0x88a8},
     {'base': 21, 'count': 3}],

    [{'base': 11, 'count': 2},
     {'base': 21, 'count': 3, 'step': 2},
     {'base': 31, 'count': 2, 'step': 3}],

    [{'base': 11, 'count': 2},
     {'base': 21, 'count': 3},
     {'base': 31, 'count': 2},
     {'base':  1, 'count': 3}],
])
def test_multiEmulDevPerVlan(request, drone, ports, dut, dut_ports, stream_id,
        emul_ports, dgid_list, vlan_cfg):
    # ----------------------------------------------------------------- #
    # TESTCASE: Emulate multiple IPv4 device per multiple single-tag VLANs
    #
    #          +- DUT -+
    #       .1/         \.1
    #        /           \
    # 10.1.1/24        10.1.2/24
    #  [vlans]          [vlans]
    #     /                  \
    #    /.101-103            \.101-103
    # Host1(s)              Host2(s)
    # ----------------------------------------------------------------- #

    num_vlans = 1
    for vcfg in vlan_cfg:
        num_vlans = num_vlans * vcfg['count']
    test_multiEmulDevPerVlan.vlan_cfg = vlan_cfg
    num_devs_per_vlan = 3
    tx_ip_base = 0x0a010165
    rx_ip_base = 0x0a010265

    # configure vlans on the DUT
    dut_vlans(request, dut_ports)

    assert len(dut_vlans.vlans) == num_vlans
    vlan_filter = []
    for i in range(len(dut_vlans.vlans)):
        vlan_filter.append('')
        ids = dut_vlans.vlans[i].split('.')
        for j in range(len(ids)):
            filter = '(frame[<ofs>:4]==<tpid>:<id>)' \
                        .replace('<ofs>', str(12+j*4)) \
                        .replace('<tpid>', '{:04x}'.format(
                            vlan_cfg[j].get('tpid', 0x8100))) \
                        .replace('<id>', '{:04x}'.format(int(ids[j])))
            if len(vlan_filter[i]) > 0:
                vlan_filter[i] += ' && '
            vlan_filter[i] += filter
        print i, vlan_filter[i]

    # configure the tx device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.tx.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.tx.device_group_id[0].id
    dg.core.name = "Host1"
    for vcfg in vlan_cfg:
        v = dg.encap.Extensions[emul.vlan].stack.add()
        v.vlan_tag = vcfg['base']
        v.count = vcfg['count']
        if 'tpid' in vcfg:
            v.tpid = vcfg['tpid']
        if 'step' in vcfg:
            v.step = vcfg['step']
    dg.device_count = num_devs_per_vlan
    dg.Extensions[emul.mac].address = 0x000102030a01
    ip = dg.Extensions[emul.ip4]
    ip.address = tx_ip_base
    ip.prefix_length = 24
    ip.default_gateway = (tx_ip_base & 0xffffff00) | 0x01

    drone.modifyDeviceGroup(devgrp_cfg)

    # configure the rx device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.rx.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.rx.device_group_id[0].id
    dg.core.name = "Host1"
    for vcfg in vlan_cfg:
        v = dg.encap.Extensions[emul.vlan].stack.add()
        v.vlan_tag = vcfg['base']
        v.count = vcfg['count']
        if 'tpid' in vcfg:
            v.tpid = vcfg['tpid']
        if 'step' in vcfg:
            v.step = vcfg['step']
    dg.device_count = num_devs_per_vlan
    dg.Extensions[emul.mac].address = 0x000102030b01
    ip = dg.Extensions[emul.ip4]
    ip.address = rx_ip_base
    ip.prefix_length = 24
    ip.default_gateway = (rx_ip_base & 0xffffff00) | 0x01

    drone.modifyDeviceGroup(devgrp_cfg)

    # configure the tx stream(s)
    # we need more than one stream, so delete old one
    # and create as many as we need
    # FIXME: restore the single stream at end?
    log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
    drone.deleteStream(stream_id)

    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.tx.port_id[0])
    for i in range(num_vlans):
        stream_id.stream_id.add().id = i
        log.info('adding tx_stream %d' % stream_id.stream_id[i].id)

    drone.addStream(stream_id)

    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.tx.port_id[0])
    for i in range(num_vlans):
        s = stream_cfg.stream.add()
        s.stream_id.id = stream_id.stream_id[i].id
        s.core.name = 'stream ' + str(s.stream_id.id)
        s.core.is_enabled = True
        s.core.ordinal = i
        s.control.packets_per_sec = 100
        s.control.num_packets = num_devs_per_vlan

        # setup stream protocols as mac:vlan:eth2:ip4:udp:payload
        p = s.protocol.add()
        p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
        p.Extensions[mac].dst_mac_mode = Mac.e_mm_resolve
        p.Extensions[mac].src_mac_mode = Mac.e_mm_resolve

        ids = dut_vlans.vlans[i].split('.')
        for id, j in zip(ids, range(len(ids))):
            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
            p.Extensions[vlan].vlan_tag = int(id)
            if 'tpid' in vlan_cfg[j]:
                p.Extensions[vlan].tpid = vlan_cfg[j]['tpid']
                p.Extensions[vlan].is_override_tpid = True

        p = s.protocol.add()
        p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

        p = s.protocol.add()
        p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
        ip = p.Extensions[ip4]
        ip.src_ip = tx_ip_base
        ip.src_ip_mode = Ip4.e_im_inc_host
        ip.src_ip_count = num_devs_per_vlan
        ip.dst_ip = rx_ip_base
        ip.dst_ip_mode = Ip4.e_im_inc_host
        ip.dst_ip_count = num_devs_per_vlan

        p = s.protocol.add()
        p.protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
        p = s.protocol.add()
        p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

        log.info('configuring tx_stream %d' % stream_id.stream_id[i].id)

    drone.modifyStream(stream_cfg)

    # clear arp on DUT
    for i in range(num_vlans):
        vrf = 'vrf' + str(i+1)

        sudo('ip netns exec ' + vrf
                + ' ip neigh flush all')
        arp_cache = sudo('ip netns exec ' + vrf
                + ' ip neigh show')
        assert re.search('10.1.[1-2].20[1-5].*lladdr', arp_cache) == None

    # resolve ARP on tx/rx ports
    log.info('resolving Neighbors on tx/rx ports ...')
    drone.startCapture(emul_ports)
    drone.clearDeviceNeighbors(emul_ports)
    drone.resolveDeviceNeighbors(emul_ports)
    time.sleep(3)
    drone.stopCapture(emul_ports)

    # verify ARP Requests sent out from tx port
    buff = drone.getCaptureBuffer(emul_ports.port_id[0])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Tx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Tx capture buffer (all pkts - vlans only)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                '-Tfields', '-eframe.number', '-eieee8021ad.id', '-evlan.id'])
    print(cap_pkts)
    log.info('dumping Tx capture buffer (filtered)')
    for i in range(num_vlans):
        for j in range(num_devs_per_vlan):
            cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                        '-R', vlan_filter[i] +
                          ' && (arp.opcode == 1)'
                          ' && (arp.src.proto_ipv4 == 10.1.1.'
                                                 +str(101+j)+')'
                          ' && (arp.dst.proto_ipv4 == 10.1.1.1)'])
            print(cap_pkts)
            assert cap_pkts.count('\n') == 1

    # verify *no* ARP Requests sent out from rx port
    buff = drone.getCaptureBuffer(emul_ports.port_id[1])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Rx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Rx capture buffer (all pkts - vlans only)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                '-Tfields', '-eframe.number', '-eieee8021ad.id', '-evlan.id'])
    print(cap_pkts)
    log.info('dumping Rx capture buffer (filtered)')
    for i in range(num_vlans):
        for j in range(num_devs_per_vlan):
            cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                        '-R', vlan_filter[i] +
                          ' && (arp.opcode == 1)'
                          ' && (arp.src.proto_ipv4 == 10.1.2.'
                                                 +str(101+j)+')'
                          ' && (arp.dst.proto_ipv4 == 10.1.2.1)'])
            print(cap_pkts)
            assert cap_pkts.count('\n') == 0

    # retrieve and verify ARP Table on tx/rx ports
    log.info('retrieving ARP entries on tx port')
    device_list = drone.getDeviceList(emul_ports.port_id[0])
    device_config = device_list.Extensions[emul.port_device]
    neigh_list = drone.getDeviceNeighbors(emul_ports.port_id[0])
    devices = neigh_list.Extensions[emul.devices]
    log.info('ARP Table on tx port')
    for dev_cfg, device in zip(device_config, devices):
        resolved = False
        for arp in device.arp:
            # TODO: print all configured vlans, not just the first
            # TODO: pretty print ip and mac
            print('v%d|%08x: %08x %012x' %
                    (dev_cfg.vlan[0] & 0xffff, dev_cfg.ip4, arp.ip4, arp.mac))
            if (arp.ip4 == dev_cfg.ip4_default_gateway) and (arp.mac):
                resolved = True
        assert resolved

    log.info('retrieving ARP entries on rx port')
    device_list = drone.getDeviceList(emul_ports.port_id[0])
    device_config = device_list.Extensions[emul.port_device]
    neigh_list = drone.getDeviceNeighbors(emul_ports.port_id[1])
    devices = neigh_list.Extensions[emul.devices]
    log.info('ARP Table on rx port')
    for dev_cfg, device in zip(device_config, devices):
        # verify *no* ARPs learnt on rx port
        assert len(device.arp) == 0
        for arp in device.arp:
            # TODO: pretty print ip and mac
            print('v%d|%08x: %08x %012x' %
                    (dev_cfg.vlan[0] & 0xffff, dev_cfg.ip4, arp.ip4, arp.mac))

    # ping the tx devices from the DUT
    for i in range(num_vlans):
        vrf = 'vrf' + str(i+1)
        for j in range(num_devs_per_vlan):
            out = sudo('ip netns exec ' + vrf
                            + ' ping -c3 10.1.1.'+str(101+j), warn_only=True)
            assert '100% packet loss' not in out

    # ping the rx devices from the DUT
    for i in range(num_vlans):
        vrf = 'vrf' + str(i+1)
        for j in range(num_devs_per_vlan):
            out = sudo('ip netns exec ' + vrf
                            + ' ping -c3 10.1.2.'+str(101+j), warn_only=True)
            assert '100% packet loss' not in out

    # we are all set - send data stream(s)
    drone.startCapture(ports.rx)
    drone.startTransmit(ports.tx)
    log.info('waiting for transmit to finish ...')
    time.sleep(5)
    drone.stopTransmit(ports.tx)
    drone.stopCapture(ports.rx)

    # verify data packets received on rx port
    buff = drone.getCaptureBuffer(ports.rx.port_id[0])
    drone.saveCaptureBuffer(buff, 'capture.pcap')
    log.info('dumping Rx capture buffer (all)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
    print(cap_pkts)
    log.info('dumping Tx capture buffer (all pkts - vlans only)')
    cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                '-Tfields', '-eframe.number', '-eieee8021ad.id', '-evlan.id'])
    print(cap_pkts)
    log.info('dumping Rx capture buffer (filtered)')
    for i in range(num_vlans):
        for j in range(num_devs_per_vlan):
            cap_pkts = subprocess.check_output(
                    [tshark, '-nr', 'capture.pcap',
                    '-R', vlan_filter[i] +
                    ' && (ip.src == 10.1.1.' + str(101+j) + ') '
                    ' && (ip.dst == 10.1.2.' + str(101+j) + ')'
                    ' && (eth.dst == 00:01:02:03:0b:'
                    + format(1+j, '02x')+')'])
            print(cap_pkts)
            assert cap_pkts.count('\n') == 1
    os.remove('capture.pcap')

import pytest
pytest.main(__file__)

