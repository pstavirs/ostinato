#! /usr/bin/env python

# standard modules
import logging
import os
import re
import subprocess
import sys
import time

from fabric.api import run, env, sudo
from harness import Test, TestSuite, TestPreRequisiteError

sys.path.insert(1, '../binding')
from core import ost_pb, emul, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac, Mac
from protocols.ip4_pb2 import ip4, Ip4
from protocols.vlan_pb2 import vlan

use_defaults = False

# initialize defaults - drone
host_name = '127.0.0.1'
tx_port_number = -1
rx_port_number = -1

if sys.platform == 'win32':
    tshark = r'C:\Program Files\Wireshark\tshark.exe'
else:
    tshark = 'tshark'

# initialize defaults - DUT
env.use_shell = False
env.user = 'tc'
env.password = 'tc'
env.host_string = 'localhost:50022'
dut_rx_port = 'eth1'
dut_tx_port = 'eth2'

dut_dst_mac = 0x0800278df2b4 #FIXME: hardcoding

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

print('')
print('This test uses the following topology -')
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

suite = TestSuite()
if not use_defaults:
    s = raw_input('Drone\'s Hostname/IP [%s]: ' % (host_name))
    host_name = s or host_name
    s = raw_input('DUT\'s Hostname/IP [%s]: ' % (env.host_string))
    env.host_string = s or env.host_string

drone = DroneProxy(host_name)

try:
    # ----------------------------------------------------------------- #
    # Baseline Configuration for subsequent testcases
    # NOTES
    #  * All test cases will emulate devices on both rx and tx ports
    #  * Each test case will create traffic streams corresponding to
    #    the devices to check
    # ----------------------------------------------------------------- #


    # FIXME: get inputs for dut rx/tx ports

    # Enable IP forwarding on the DUT (aka make it a router)
    sudo('sysctl -w net.ipv4.ip_forward=1')

    # connect to drone
    log.info('connecting to drone(%s:%d)'
            % (drone.hostName(), drone.portNumber()))
    drone.connect()

    # retreive port id list
    log.info('retreiving port list')
    port_id_list = drone.getPortIdList()

    # retreive port config list
    log.info('retreiving port config for all ports')
    port_config_list = drone.getPortConfig(port_id_list)

    if len(port_config_list.port) == 0:
        log.warning('drone has no ports!')
        sys.exit(1)

    # print port list and find default tx/rx ports
    print('Port List')
    print('---------')
    for port in port_config_list.port:
        print('%d.%s (%s)' % (port.port_id.id, port.name, port.description))
        # use a vhost port as default tx/rx port
        if ('vhost' in port.name or 'sun' in port.description.lower()):
            if tx_port_number < 0:
                tx_port_number = port.port_id.id
            elif rx_port_number < 0:
                rx_port_number = port.port_id.id
        if ('eth1' in port.name):
            tx_port_number = port.port_id.id
        if ('eth2' in port.name):
            rx_port_number = port.port_id.id

    if not use_defaults:
        p = raw_input('Tx Port Id [%d]: ' % (tx_port_number))
        if p:
            tx_port_number = int(p)

        p = raw_input('Rx Port Id [%d]: ' % (rx_port_number))
        if p:
            rx_port_number = int(p)

    if tx_port_number < 0 or rx_port_number < 0:
        log.warning('invalid tx/rx port')
        sys.exit(1)

    print('Using port %d as tx port(s)' % tx_port_number)
    print('Using port %d as rx port(s)' % rx_port_number)

    tx_port = ost_pb.PortIdList()
    tx_port.port_id.add().id = tx_port_number;

    rx_port = ost_pb.PortIdList()
    rx_port.port_id.add().id = rx_port_number;

    # ----------------------------------------------------------------- #
    # create emulated device(s) on tx/rx ports - each test case will
    # modify and reuse these devices as per its needs
    # ----------------------------------------------------------------- #

    emul_ports = ost_pb.PortIdList()
    emul_ports.port_id.add().id = tx_port_number;
    emul_ports.port_id.add().id = rx_port_number;

    # delete existing devices, if any, on tx port
    tx_dgid_list = drone.getDeviceGroupIdList(tx_port.port_id[0])
    drone.deleteDeviceGroup(tx_dgid_list)

    # add a emulated device on tx port
    tx_dgid_list = ost_pb.DeviceGroupIdList()
    tx_dgid_list.port_id.CopyFrom(tx_port.port_id[0])
    tx_dgid_list.device_group_id.add().id = 1
    log.info('adding tx device_group %d' % tx_dgid_list.device_group_id[0].id)
    drone.addDeviceGroup(tx_dgid_list)

    # delete existing devices, if any, on rx port
    rx_dgid_list = drone.getDeviceGroupIdList(rx_port.port_id[0])
    drone.deleteDeviceGroup(rx_dgid_list)

    # add a emulated device on rx port
    rx_dgid_list = ost_pb.DeviceGroupIdList()
    rx_dgid_list.port_id.CopyFrom(rx_port.port_id[0])
    rx_dgid_list.device_group_id.add().id = 1
    log.info('adding rx device_group %d' % rx_dgid_list.device_group_id[0].id)
    drone.addDeviceGroup(rx_dgid_list)

    # ----------------------------------------------------------------- #
    # create stream on tx port - each test case will modify and reuse
    # this stream as per its needs
    # ----------------------------------------------------------------- #

    # delete existing streams, if any, on tx port
    sid_list = drone.getStreamIdList(tx_port.port_id[0])
    drone.deleteStream(sid_list)

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(tx_port.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    # ----------------------------------------------------------------- #
    # delete all configuration on the DUT interfaces
    # ----------------------------------------------------------------- #
    sudo('ip address flush dev ' + dut_rx_port)
    sudo('ip address flush dev ' + dut_tx_port)

    # ================================================================= #
    # ----------------------------------------------------------------- #
    #                            TEST CASES
    # ----------------------------------------------------------------- #
    # ================================================================= #

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

    passed = False
    suite.test_begin('multiEmulDevNoVlan')

    num_devs = 5
    try:
        # configure the DUT
        sudo('ip address add 10.10.1.1/24 dev ' + dut_rx_port)
        sudo('ip address add 10.10.2.1/24 dev ' + dut_tx_port)

        # configure the tx device(s)
        devgrp_cfg = ost_pb.DeviceGroupConfigList()
        devgrp_cfg.port_id.CopyFrom(tx_port.port_id[0])
        dg = devgrp_cfg.device_group.add()
        dg.device_group_id.id = tx_dgid_list.device_group_id[0].id
        dg.core.name = "Host1"
        dg.device_count = num_devs
        dg.Extensions[emul.mac].address = 0x000102030a01
        ip = dg.Extensions[emul.ip4]
        ip.address = 0x0a0a0165
        ip.prefix_length = 24
        ip.default_gateway = 0x0a0a0101

        drone.modifyDeviceGroup(devgrp_cfg)

        # configure the rx device(s)
        devgrp_cfg = ost_pb.DeviceGroupConfigList()
        devgrp_cfg.port_id.CopyFrom(rx_port.port_id[0])
        dg = devgrp_cfg.device_group.add()
        dg.device_group_id.id = rx_dgid_list.device_group_id[0].id
        dg.core.name = "Host1"
        dg.device_count = num_devs
        dg.Extensions[emul.mac].address = 0x000102030b01
        ip = dg.Extensions[emul.ip4]
        ip.address = 0x0a0a0265
        ip.prefix_length = 24
        ip.default_gateway = 0x0a0a0201

        drone.modifyDeviceGroup(devgrp_cfg)

        # configure the tx stream
        stream_cfg = ost_pb.StreamConfigList()
        stream_cfg.port_id.CopyFrom(tx_port.port_id[0])
        s = stream_cfg.stream.add()
        s.stream_id.id = stream_id.stream_id[0].id
        s.core.is_enabled = True
        s.control.packets_per_sec = 20
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
        ip.src_ip_mode = Ip4.e_im_inc_host
        ip.src_ip_count = num_devs
        ip.dst_ip = 0x0a0a0265
        ip.dst_ip_mode = Ip4.e_im_inc_host
        ip.dst_ip_count = num_devs

        s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
        s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

        log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
        drone.modifyStream(stream_cfg)

        # clear tx/rx stats
        log.info('clearing tx/rx stats')
        drone.clearStats(tx_port)
        drone.clearStats(rx_port)

        # clear arp on DUT
        sudo('ip neigh flush all')
        arp_cache = run('ip neigh show')
        if re.search('10.10.2.10[1-5].*lladdr', arp_cache):
            raise TestPreRequisiteError('ARP cache not cleared')

        # resolve ARP on tx/rx ports
        log.info('resolving Neighbors on tx/rx ports ...')
        drone.startCapture(emul_ports)
        drone.clearDeviceNeighbors(emul_ports)
        drone.resolveDeviceNeighbors(emul_ports)
        time.sleep(10)
        drone.stopCapture(emul_ports)

        fail = 0

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
                          ' && (arp.src.proto_ipv4 == 10.10.1.'+str(101+i)+')'
                          ' && (arp.dst.proto_ipv4 == 10.10.1.1)'])
            print(cap_pkts)
            if cap_pkts.count('\n') != 1:
                fail = fail + 1

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
                          ' && (arp.src.proto_ipv4 == 10.10.2.'+str(101+i)+')'
                          ' && (arp.dst.proto_ipv4 == 10.10.2.1)'])
            print(cap_pkts)
            if cap_pkts.count('\n') != 0:
                fail = fail + 1

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
            if not resolved:
                fail = fail + 1

        log.info('retrieving ARP entries on rx port')
        device_list = drone.getDeviceList(emul_ports.port_id[0])
        device_config = device_list.Extensions[emul.port_device]
        neigh_list = drone.getDeviceNeighbors(emul_ports.port_id[1])
        devices = neigh_list.Extensions[emul.devices]
        log.info('ARP Table on rx port')
        for dev_cfg, device in zip(device_config, devices):
            # verify *no* ARPs learnt on rx port
            if len(device.arp):
                fail = fail + 1
            for arp in device.arp:
                # TODO: pretty print ip and mac
                print('%08x: %08x %012x' %
                        (dev_cfg.ip4, arp.ip4, arp.mac))

        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(7)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer (all)')
        cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
        print(cap_pkts)
        log.info('dumping Rx capture buffer (filtered)')
        for i in range(num_devs):
            cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap',
                        '-R', '(ip.src == 10.10.1.' + str(101+i) + ') '
                          ' && (ip.dst == 10.10.2.' + str(101+i) + ')'
                          ' && (eth.dst == 00:01:02:03:0b:'
                                    + format(1+i, '02x')+')'])
            print(cap_pkts)
            if cap_pkts.count('\n') != s.control.num_packets/ip.src_ip_count:
                fail = fail + 1
        if fail == 0:
            passed = True
        else:
            log.info('failed checks: %d' % fail)
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        run('ip neigh show')
        # unconfigure DUT
        sudo('ip address delete 10.10.1.1/24 dev ' + dut_rx_port)
        sudo('ip address delete 10.10.2.1/24 dev ' + dut_tx_port)
        suite.test_end(passed)

    sys.exit(1)
    # FIXME: update the below test cases to resolve Neighbors and streams
    # to derive src/dst mac from device

    # ----------------------------------------------------------------- #
    # TESTCASE: Emulate multiple IPv4 device per multiple single-tag VLANs
    #
    #          +- DUT -+
    #       .1/         \.1
    #        /           \
    # 10.1.1/24        10.1.2/24
    #  v[201-205]      v[201-205]
    #     /                  \
    #    /.101-103            \.101-103
    # Host1(s)              Host2(s)
    # ----------------------------------------------------------------- #

    passed = False
    suite.test_begin('multiEmulDevPerVlan')

    num_vlans = 5
    vlan_base = 201
    num_devs = 3
    dev_ip_base = 101
    try:
        # configure the DUT
        for i in range(num_vlans):
            vlan_id = vlan_base+i
            vrf = 'v' + str(vlan_id)
            vlan_rx_dev = dut_rx_port + '.' + str(vlan_id)
            vlan_tx_dev = dut_tx_port + '.' + str(vlan_id)

            sudo('ip netns add ' + vrf)

            sudo('ip link add link ' + dut_rx_port
                 + ' name ' + vlan_rx_dev
                 + ' type vlan id ' + str(vlan_id))
            sudo('ip link set ' + vlan_rx_dev
                    + ' netns ' + vrf)
            sudo('ip netns exec ' + vrf
                    + ' ip addr add 10.1.1.1/24'
                    + ' dev ' + vlan_rx_dev)
            sudo('ip netns exec ' + vrf
                    + ' ip link set ' + vlan_rx_dev + ' up')

            sudo('ip link add link ' + dut_tx_port
                 + ' name ' + vlan_tx_dev
                 + ' type vlan id ' + str(vlan_id))
            sudo('ip link set ' + vlan_tx_dev
                    + ' netns ' + vrf)
            sudo('ip netns exec ' + vrf
                    + ' ip addr add 10.1.2.1/24'
                    + ' dev ' + vlan_tx_dev)
            sudo('ip netns exec ' + vrf
                    + ' ip link set ' + vlan_tx_dev + ' up')


        # configure the tx device(s)
        devgrp_cfg = ost_pb.DeviceGroupConfigList()
        devgrp_cfg.port_id.CopyFrom(tx_port.port_id[0])
        dg = devgrp_cfg.device_group.add()
        dg.device_group_id.id = tx_dgid_list.device_group_id[0].id
        dg.core.name = "Host1"
        v = dg.Extensions[emul.vlan].stack.add()
        v.vlan_tag = vlan_base
        v.count = num_vlans
        d = dg.Extensions[emul.device]
        d.count = num_devs
        d.mac.address = 0x000102030a01
        d.ip4.address = 0x0a010165
        d.ip4.prefix_length = 24
        d.ip4.default_gateway = 0x0a0a0101

        drone.modifyDeviceGroup(devgrp_cfg)

        # configure the rx device(s)
        devgrp_cfg = ost_pb.DeviceGroupConfigList()
        devgrp_cfg.port_id.CopyFrom(rx_port.port_id[0])
        dg = devgrp_cfg.device_group.add()
        dg.device_group_id.id = rx_dgid_list.device_group_id[0].id
        dg.core.name = "Host1"
        v = dg.Extensions[emul.vlan].stack.add()
        v.vlan_tag = vlan_base
        v.count = num_vlans
        d = dg.Extensions[emul.device]
        d.count = num_devs
        #d.mode = emul.Device.kNoRepeat
        d.mac.address = 0x000102030b01
        d.ip4.address = 0x0a010265
        d.ip4.prefix_length = 24
        d.ip4.default_gateway = 0x0a0a0201

        drone.modifyDeviceGroup(devgrp_cfg)

        # configure the tx stream(s)
        # we need more than one stream, so delete old one
        # and create as many as we need
        # FIXME: restore the single stream at end?
        log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
        drone.deleteStream(stream_id)

        stream_id = ost_pb.StreamIdList()
        stream_id.port_id.CopyFrom(tx_port.port_id[0])
        for i in range(num_vlans):
            stream_id.stream_id.add().id = i
            log.info('adding tx_stream %d' % stream_id.stream_id[i].id)

        drone.addStream(stream_id)

        stream_cfg = ost_pb.StreamConfigList()
        stream_cfg.port_id.CopyFrom(tx_port.port_id[0])
        for i in range(num_vlans):
            s = stream_cfg.stream.add()
            s.stream_id.id = stream_id.stream_id[i].id
            s.core.is_enabled = True
            s.core.ordinal = i
            s.control.packets_per_sec = 10
            s.control.num_packets = num_devs

            # setup stream protocols as mac:vlan:eth2:ip4:udp:payload
            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
            p.Extensions[mac].dst_mac = dut_dst_mac
            p.Extensions[mac].src_mac = 0x00aabbccddee

            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
            p.Extensions[vlan].vlan_tag = vlan_base+i

            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
            ip = p.Extensions[ip4]
            ip.src_ip = 0x0a010165
            ip.src_ip_mode = Ip4.e_im_inc_host
            ip.dst_ip = 0x0a010265
            ip.dst_ip_mode = Ip4.e_im_inc_host

            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

            log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)

        drone.modifyStream(stream_cfg)

        # clear tx/rx stats
        log.info('clearing tx/rx stats')
        drone.clearStats(tx_port)
        drone.clearStats(rx_port)

        # clear arp on DUT
        for i in range(num_vlans):
            vlan_id = vlan_base + i
            vrf = 'v' + str(vlan_id)
            vlan_rx_dev = dut_rx_port + '.' + str(vlan_id)
            vlan_tx_dev = dut_tx_port + '.' + str(vlan_id)

            sudo('ip netns exec ' + vrf
                    + ' ip neigh flush dev ' + vlan_rx_dev)
            sudo('ip netns exec ' + vrf
                    + ' ip neigh flush dev ' + vlan_tx_dev)
            sudo('ip netns exec ' + vrf
                    + ' ip neigh show')

        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(5)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer (all)')
        cap_pkts = subprocess.check_output([tshark, '-nr', 'capture.pcap'])
        print(cap_pkts)
        log.info('dumping Rx capture buffer (filtered)')
        fail = 0
        for i in range(num_vlans):
            for j in range(num_devs):
                cap_pkts = subprocess.check_output(
                        [tshark, '-nr', 'capture.pcap',
                        '-R', '(vlan.id == ' + str(201+i) + ')'
                        ' && (ip.src == 10.1.1.' + str(101+j) + ') '
                        ' && (ip.dst == 10.1.2.' + str(101+j) + ')'
                        ' && (eth.dst == 00:01:02:03:0b:'
                        + format(1+j, '02x')+')'])
                print(cap_pkts)
                if cap_pkts.count('\n') != 1:
                    fail = fail + 1
        if fail == 0:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        # show arp on DUT
        for i in range(num_vlans):
            vrf = 'v' + str(vlan_base + i)
            sudo('ip netns exec ' + vrf
                    + ' ip neigh show')
        # un-configure the DUT
        for i in range(num_vlans):
            vlan_id = vlan_base + i
            vrf = 'v' + str(vlan_id)
            sudo('ip netns delete ' + vrf)
        suite.test_end(passed)

    # TODO:
    # ----------------------------------------------------------------- #
    # TESTCASE: Emulate one IPv4 device per multiple double-tag VLANs
    #           vlanMode: repeat (default)
    # TESTCASE: Emulate multiple IPv4 devices per multiple double-tag VLANs
    #           vlanMode: no-repeat; ip4Mode: repeat (default)
    # TESTCASE: Emulate multiple IPv4 devices per multiple quad-tag VLANs
    #           vlanMode: repeat (default); ip4Mode: repeat (default)
    # TESTCASE: Emulate multiple IPv4 devices per multiple quad-tag VLANs
    #           vlanMode: no-repeat; ip4Mode: no-repeat
    # ----------------------------------------------------------------- #

    suite.complete()

    # delete stream(s)
    log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
    drone.deleteStream(stream_id)

    # delete device(s)
    dgid_list = drone.getDeviceGroupIdList(tx_port.port_id[0])
    drone.deleteDeviceGroup(dgid_list)
    dgid_list = drone.getDeviceGroupIdList(rx_port.port_id[0])
    drone.deleteDeviceGroup(dgid_list)

    # bye for now
    drone.disconnect()
    #disconnect_all()

except Exception as ex:
    log.exception(ex)

finally:
    suite.report()
    if not suite.passed:
        sys.exit(2);
