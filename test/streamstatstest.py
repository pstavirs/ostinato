#! /usr/bin/env python

# standard modules
import ipaddress
import logging
import os
import pytest
import random
import re
import subprocess
import sys
import time

import pytest
from fabric.api import run, env, sudo

from utils import get_tshark

sys.path.insert(1, '../binding')
from core import ost_pb, emul, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac, Mac
from protocols.ip4_pb2 import ip4, Ip4
from protocols.ip6_pb2 import ip6, Ip6
from protocols.payload_pb2 import payload, Payload
from protocols.sign_pb2 import sign

# Convenience class to interwork with OstEmul::Ip6Address() and
# the python ipaddress module
# FIXME: move to a common module for reuse and remove duplication in other
#        scripts
class ip6_address(ipaddress.IPv6Interface):
    def __init__(self, addr):
        if type(addr) is str:
            super(ip6_address, self).__init__(unicode(addr))
        elif type(addr) is int:
            super(ip6_address, self).__init__(addr)
        else:
            super(ip6_address, self).__init__(addr.hi << 64 | addr.lo)
        self.ip6 = emul.Ip6Address()
        self.ip6.hi = int(self) >> 64
        self.ip6.lo = int(self) & 0xffffffffffffffff

        self.prefixlen = self.network.prefixlen

        # we assume gateway is the lowest IP host address in the network
        gateway = self.network.network_address + 1
        self.gateway = emul.Ip6Address()
        self.gateway.hi = int(gateway) >> 64
        self.gateway.lo = int(gateway) & 0xffffffffffffffff

use_defaults = True

# initialize defaults
host_name = '127.0.0.1'

# initialize defaults - DUT
env.use_shell = False
env.user = 'tc'
env.password = 'tc'
env.host_string = 'localhost:50022'

tshark = get_tshark(minversion = '2')

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

print(' +-------+          +-------+')
print(' |       |X--<-->--X|-+     |')
print(' | Drone |          | | DUT |')
print(' |       |Y--<-->--Y|-+     |')
print(' +-------+          +-------+')
print('')
print('Drone has 2 ports connected to DUT. Packets sent on port X')
print('are expected to be forwarded by the DUT and received back on')
print('port Y and vice versa')
print('')

if not use_defaults:
    s = raw_input('Drone\'s Hostname/IP [%s]: ' % (host_name))
    host_name = s or host_name
    s = raw_input('DUT\'s Hostname/IP [%s]: ' % (env.host_string))
    env.host_string = s or env.host_string
    # FIXME: get inputs for dut x/y ports

@pytest.fixture(scope='module')
def drone(request):
    """Baseline Configuration for all testcases in this module"""

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

    # print port list and find default X/Y ports
    ports.x_num = -1
    ports.y_num = -1
    print port_config_list
    print('Port List')
    print('---------')
    for port in port_config_list.port:
        print('%d.%s (%s)' % (port.port_id.id, port.name, port.description))
        # use a vhost port as default X/Y port
        if ('vhost' in port.name or 'oracle' in port.description.lower()):
            if ports.x_num < 0:
                ports.x_num = port.port_id.id
            elif ports.y_num < 0:
                ports.y_num = port.port_id.id
        if ('eth1' in port.name):
            ports.x_num = port.port_id.id
        if ('eth2' in port.name):
            ports.y_num = port.port_id.id

    assert ports.x_num >= 0
    assert ports.y_num >= 0

    print('Using port %d as port X' % ports.x_num)
    print('Using port %d as port Y' % ports.y_num)

    ports.x = ost_pb.PortIdList()
    ports.x.port_id.add().id = ports.x_num;

    ports.y = ost_pb.PortIdList()
    ports.y.port_id.add().id = ports.y_num;

    # Enable stream stats on ports
    portConfig = ost_pb.PortConfigList()
    portConfig.port.add().port_id.id = ports.x_num;
    portConfig.port[0].is_tracking_stream_stats = True;
    portConfig.port.add().port_id.id = ports.y_num;
    portConfig.port[1].is_tracking_stream_stats = True;
    print('Enabling Stream Stats tracking on ports X and Y');
    drone.modifyPort(portConfig);

    return ports

@pytest.fixture(scope='module')
def emul_ports(request, drone, ports):
    emul_ports = ost_pb.PortIdList()
    emul_ports.port_id.add().id = ports.x.port_id[0].id;
    emul_ports.port_id.add().id = ports.y.port_id[0].id;
    return emul_ports

@pytest.fixture(scope='module')
def dgid_list(request, drone, ports):
    # ----------------------------------------------------------------- #
    # create emulated device(s) on tx/rx ports - each test case will
    # use these same devices
    # ----------------------------------------------------------------- #

    # delete existing devices, if any, on tx port
    dgid_list.x = drone.getDeviceGroupIdList(ports.x.port_id[0])
    drone.deleteDeviceGroup(dgid_list.x)

    # add a emulated device group on port X
    dgid_list.x = ost_pb.DeviceGroupIdList()
    dgid_list.x.port_id.CopyFrom(ports.x.port_id[0])
    dgid_list.x.device_group_id.add().id = 1
    log.info('adding X device_group %d' % dgid_list.x.device_group_id[0].id)
    drone.addDeviceGroup(dgid_list.x)

    # configure the X device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.x.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.x.device_group_id[0].id
    dg.core.name = "HostX"
    dg.device_count = 5

    dg.Extensions[emul.mac].address = 0x000102030a01

    dg.Extensions[emul.ip4].address = 0x0a0a0165
    dg.Extensions[emul.ip4].prefix_length = 24
    dg.Extensions[emul.ip4].default_gateway = 0x0a0a0101

    ip6addr = ip6_address('1234:1::65/96')
    dg.Extensions[emul.ip6].address.CopyFrom(ip6addr.ip6)
    dg.Extensions[emul.ip6].prefix_length = ip6addr.prefixlen
    dg.Extensions[emul.ip6].default_gateway.CopyFrom(ip6addr.gateway)

    drone.modifyDeviceGroup(devgrp_cfg)

    # delete existing devices, if any, on Y port
    dgid_list.y = drone.getDeviceGroupIdList(ports.y.port_id[0])
    drone.deleteDeviceGroup(dgid_list.y)

    # add a emulated device group on port Y
    dgid_list.y = ost_pb.DeviceGroupIdList()
    dgid_list.y.port_id.CopyFrom(ports.y.port_id[0])
    dgid_list.y.device_group_id.add().id = 1
    log.info('adding Y device_group %d' % dgid_list.y.device_group_id[0].id)
    drone.addDeviceGroup(dgid_list.y)

    # configure the Y device(s)
    devgrp_cfg = ost_pb.DeviceGroupConfigList()
    devgrp_cfg.port_id.CopyFrom(ports.y.port_id[0])
    dg = devgrp_cfg.device_group.add()
    dg.device_group_id.id = dgid_list.y.device_group_id[0].id
    dg.core.name = "HostY"

    dg.Extensions[emul.mac].address = 0x000102030b01

    dg.Extensions[emul.ip4].address = 0x0a0a0265
    dg.Extensions[emul.ip4].prefix_length = 24
    dg.Extensions[emul.ip4].default_gateway = 0x0a0a0201

    ip6addr = ip6_address('1234:2::65/96')
    dg.Extensions[emul.ip6].address.CopyFrom(ip6addr.ip6)
    dg.Extensions[emul.ip6].prefix_length = ip6addr.prefixlen
    dg.Extensions[emul.ip6].default_gateway.CopyFrom(ip6addr.gateway)

    drone.modifyDeviceGroup(devgrp_cfg)

    def fin():
        dgid_list = drone.getDeviceGroupIdList(ports.x.port_id[0])
        drone.deleteDeviceGroup(dgid_list)
        dgid_list = drone.getDeviceGroupIdList(ports.y.port_id[0])
        drone.deleteDeviceGroup(dgid_list)
    request.addfinalizer(fin)

    return dgid_list

@pytest.fixture(scope='module')
def dut(request):
    # Enable IP forwarding on the DUT (aka make it a router)
    sudo('sysctl -w net.ipv4.ip_forward=1')
    sudo('sysctl -w net.ipv6.conf.all.forwarding=1')

@pytest.fixture(scope='module')
def dut_ports(request):
    dut_ports.x = 'eth1'
    dut_ports.y = 'eth2'

    # delete all configuration on the DUT interfaces
    sudo('ip address flush dev ' + dut_ports.y)
    sudo('ip address flush dev ' + dut_ports.x)
    return dut_ports

@pytest.fixture
def dut_ip(request, dut_ports):
    sudo('ip address add 10.10.1.1/24 dev ' + dut_ports.x)
    sudo('ip address add 10.10.2.1/24 dev ' + dut_ports.y)

    sudo('ip -6 address add 1234:1::1/96 dev ' + dut_ports.x)
    sudo('ip -6 address add 1234:2::1/96 dev ' + dut_ports.y)

    def fin():
        sudo('ip address delete 10.10.1.1/24 dev ' + dut_ports.x)
        sudo('ip address delete 10.10.2.1/24 dev ' + dut_ports.y)

        sudo('ip -6 address delete 1234:1::1/96 dev ' + dut_ports.x)
        sudo('ip -6 address delete 1234:2::1/96 dev ' + dut_ports.y)
    request.addfinalizer(fin)

@pytest.fixture(scope='module')
def stream_clear(request, drone, ports):
    # delete existing streams, if any, on all ports
    sid_list = drone.getStreamIdList(ports.x.port_id[0])
    drone.deleteStream(sid_list)
    sid_list = drone.getStreamIdList(ports.y.port_id[0])
    drone.deleteStream(sid_list)

@pytest.fixture
def stream(request, drone, ports, sign_stream_cfg):

    sscfg = sign_stream_cfg
    num_streams = sscfg['num_streams']

    # add stream(s)
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.x.port_id[0])
    for i in range(num_streams):
        stream_id.stream_id.add().id = 1+i
    log.info('adding X stream(s) ' + str(stream_id))
    drone.addStream(stream_id)

    # configure the stream(s)
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.x.port_id[0])
    for i in range(num_streams):
        s = stream_cfg.stream.add()
        s.stream_id.id = stream_id.stream_id[i].id
        s.core.is_enabled = True
        # On Win32, packets in a seq that take less than 1s to transmit
        # are transmitted in one shot using pcap_send_queue; this means that
        # stopTransmit() cannot stop them in the middle of the sequence and
        # stop will happen only at the pktSeq boundary
        # Since we want to test cases where stop is trigerred in the middle of
        # a sequence, we set pps < 1000
        # FIXME: for some reason values such as 100pps is also leading to
        # stop only at seq boundary - need to debug
        s.control.packets_per_sec = 523

        # setup stream protocols
        p = s.protocol.add()
        s.control.num_packets = sscfg['num_pkts'][i]
        p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
        p.Extensions[mac].dst_mac_mode = Mac.e_mm_resolve
        p.Extensions[mac].src_mac_mode = Mac.e_mm_resolve

        s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

        p = s.protocol.add()
        if i & 0x1: # odd numbered stream
            p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
            ip = p.Extensions[ip4]
            ip.src_ip = 0x0a0a0165
            ip.dst_ip = 0x0a0a0265
            if sscfg['num_var_pkts'][i]:
                ip4.src_ip_mode = Ip4.e_im_inc_host
                ip4.src_ip_count = sscfg['num_var_pkts'][i]
            s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
        else:       # even numbered stream
            s.core.frame_len = 96 # IPv6 needs a larger frame
            p.protocol_id.id = ost_pb.Protocol.kIp6FieldNumber
            ip = p.Extensions[ip6]
            ip6addr = ip6_address('1234:1::65/96')
            ip.src_addr_hi = ip6addr.ip6.hi
            ip.src_addr_lo = ip6addr.ip6.lo
            ip6addr = ip6_address('1234:2::65/96')
            ip.dst_addr_hi = ip6addr.ip6.hi
            ip.dst_addr_lo = ip6addr.ip6.lo
            if sscfg['num_var_pkts'][i]:
                ip.dst_addr_mode = Ip6.kIncHost
                ip.dst_addr_count = sscfg['num_var_pkts'][i]
                ip.dst_addr_prefix = ip6addr.prefixlen
            s.protocol.add().protocol_id.id = ost_pb.Protocol.kTcpFieldNumber

        s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
        if sscfg['guid'][i] > 0:
            p = s.protocol.add()
            p.protocol_id.id = ost_pb.Protocol.kSignFieldNumber
            p.Extensions[sign].stream_guid = sscfg['guid'][i]

    if sscfg['loop']:
        stream_cfg.stream[-1].control.next = ost_pb.StreamControl.e_nw_goto_id

    drone.modifyStream(stream_cfg)

    def fin():
        # delete streams
        log.info('deleting tx_stream ' + str(stream_id))
        drone.deleteStream(stream_id)

    request.addfinalizer(fin)

    return stream_cfg

@pytest.fixture(scope='module')
def stream_guids(request, drone, ports):
    stream_guids = ost_pb.StreamGuidList()
    stream_guids.port_id_list.port_id.add().id = ports.x.port_id[0].id;
    stream_guids.port_id_list.port_id.add().id = ports.y.port_id[0].id;
    return stream_guids

# ================================================================= #
# ----------------------------------------------------------------- #
#                            TEST CASES
# ----------------------------------------------------------------- #
# ================================================================= #

@pytest.mark.parametrize('sign_stream_cfg', [
    # Min Seq Size = 256 on Win32 and 16 on other platforms, keep this in
    # mind while configuring the below num_pkts
    # Verify tx pkts = multiple complete repeats of packetList + no partials
    {'num_streams': 3, 'num_pkts': [10, 270, 512], 'num_var_pkts': [0, 0, 0],
        'guid': [-1, 101, 102], 'loop': False},
    # Verify tx pkts = multiple repeats of packetList + partial repeat
    {'num_streams': 3, 'num_pkts': [10, 276, 510], 'num_var_pkts': [0, 0, 0],
        'guid': [-1, 101, 102], 'loop': True},
    # Verify tx pkts = first seq partial
    {'num_streams': 1, 'num_pkts': [12], 'num_var_pkts': [0],
        'guid': [101], 'loop': True},
])
def test_unidir(drone, ports, dut, dut_ports, dut_ip, emul_ports, dgid_list,
        sign_stream_cfg, stream_clear, stream, stream_guids):
    """ TESTCASE: Verify that uni-directional stream stats are correct for a
        single signed stream X --> Y
                     DUT
                   /.1   \.1
                  /       \
            10.10.1/24  10.10.2/24
            1234::1/96  1234::2/96
                /           \
               /.101-105     \.101-105
            HostX           HostY
     """

    # calculate tx pkts
    total_tx_pkts = 0
    for pkts in sign_stream_cfg['num_pkts']:
        total_tx_pkts += pkts

    num_sign_streams = sum(1 for i in sign_stream_cfg['guid'] if i > 0)

    # clear port X/Y stats
    log.info('clearing stats')
    drone.clearStats(ports.x)
    drone.clearStats(ports.y)

    # clear and verify stream strats are indeed cleared
    drone.clearStreamStats(stream_guids)
    ssd = drone.getStreamStatsDict(stream_guids)
    assert len(ssd.port) == 0

    # resolve ARP/NDP on ports X/Y
    log.info('resolving Neighbors on (X, Y) ports ...')
    # wait for interface to do DAD? Otherwise we don't get replies for NS
    # FIXME: find alternative to sleep
    time.sleep(5)
    drone.resolveDeviceNeighbors(emul_ports)
    time.sleep(3)

    # clear ARP/NDP cache before ping from DUT to devices
    #  - this helps set up ARP/NDP on DUT so that it doesn't age out
    #    while traffic is flowing
    sudo('ip neigh flush all')
    out = run('ping -c3 10.10.1.101', warn_only=True)
    assert '100% packet loss' not in out
    out = run('ping -c3 10.10.2.101', warn_only=True)
    assert '100% packet loss' not in out
    out = run('ping -6 -c3 1234:1::65', warn_only=True)
    assert '100% packet loss' not in out
    out = run('ping -6 -c3 1234:2::65', warn_only=True)
    assert '100% packet loss' not in out

    # FIXME: dump ARP/NDP table on devices and DUT

    try:
        #run('ip -6 neigh show')

        drone.startCapture(ports.y)
        drone.startCapture(ports.x)
        time.sleep(1)
        drone.startTransmit(ports.x)
        log.info('waiting for transmit to finish ...')
        time.sleep(random.randint(3, 10))
        drone.stopTransmit(ports.x)
        time.sleep(1)
        drone.stopCapture(ports.x)
        drone.stopCapture(ports.y)

        #run('ip -6 neigh show')

        # verify port stats
        x_stats = drone.getStats(ports.x)
        log.info('--> (x_stats)' + x_stats.__str__())
        if not sign_stream_cfg['loop']:
            assert(x_stats.port_stats[0].tx_pkts >= total_tx_pkts)

        y_stats = drone.getStats(ports.y)
        log.info('--> (y_stats)' + y_stats.__str__())
        if not sign_stream_cfg['loop']:
            assert(y_stats.port_stats[0].rx_pkts >= total_tx_pkts)

        ssd = drone.getStreamStatsDict(stream_guids)
        log.info('--> (stream stats)\n' + str(ssd))
        assert len(ssd.port) == 2
        assert len(ssd.port[ports.x_num].sguid) == num_sign_streams
        assert len(ssd.port[ports.y_num].sguid) == num_sign_streams
        assert len(ssd.sguid) == num_sign_streams

        # dump X capture buffer
        log.info('getting X capture buffer')
        buff = drone.getCaptureBuffer(ports.x.port_id[0])
        drone.saveCaptureBuffer(buff, 'capX.pcap')
        #log.info('dumping X capture buffer')
        #cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capX.pcap'])
        #print(cap_pkts)

        # get and verify each sign stream Tx stats from capture
        for i in range(sign_stream_cfg['num_streams']):
            guid = sign_stream_cfg['guid'][i]
            if guid < 0:
                continue
            filter='frame[-9:9]==00.' \
                        + re.sub('..', '\g<0>.', format(guid, '06x')) \
                        + '61.1d.10.c0.da && !icmp && !icmpv6'
            print(filter)
            cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capX.pcap',
                '-Y', filter])
            #log.info('dumping X capture buffer (filtered)')
            #print(cap_pkts)
            sign_stream_cnt = cap_pkts.count('\n')

            log.info('guid %d tx cap count: %d' % (guid, sign_stream_cnt))
            #log.info('--> (stream stats)\n' + str(ssd))

            # verify tx stream stats from drone is same as that from capture
            assert ssd.port[ports.x_num].sguid[guid].tx_pkts \
                    == sign_stream_cnt
            assert ssd.port[ports.x_num].sguid[guid].tx_bytes \
                    == (sign_stream_cnt
                            * (stream.stream[i].core.frame_len - 4))

            # verify tx stream stats from drone is same as configured
            if not sign_stream_cfg['loop']:
                assert ssd.port[ports.x_num].sguid[guid].tx_pkts \
                        == sign_stream_cfg['num_pkts'][i]
                assert ssd.port[ports.x_num].sguid[guid].tx_bytes \
                        == (sign_stream_cfg['num_pkts'][i]
                                * (stream.stream[i].core.frame_len - 4))
        os.remove('capX.pcap')

        # dump Y capture buffer
        log.info('getting Y capture buffer')
        buff = drone.getCaptureBuffer(ports.y.port_id[0])
        drone.saveCaptureBuffer(buff, 'capY.pcap')
        #log.info('dumping Y capture buffer')
        #cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capY.pcap'])
        #print(cap_pkts)

        # get and verify each sign stream Rx stats from capture
        for i in range(sign_stream_cfg['num_streams']):
            guid = sign_stream_cfg['guid'][i]
            if guid < 0:
                continue
            filter='frame[-9:9]==00.' \
                        + re.sub('..', '\g<0>.', format(guid, '06x')) \
                        + '61.1d.10.c0.da && !icmp && !icmpv6'
            print(filter)
            cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capY.pcap',
                '-Y', filter])
            #log.info('dumping X capture buffer (filtered)')
            #print(cap_pkts)
            sign_stream_cnt = cap_pkts.count('\n')

            log.info('guid %d rx cap count: %d' % (guid, sign_stream_cnt))
            #log.info('--> (stream stats)\n' + str(ssd))

            # verify rx stream stats from drone is same as that from capture
            assert ssd.port[ports.y_num].sguid[guid].rx_pkts \
                    == sign_stream_cnt
            assert ssd.port[ports.y_num].sguid[guid].rx_bytes \
                    == (sign_stream_cnt
                            * (stream.stream[i].core.frame_len - 4))

            # verify rx stream stats from drone is same as configured
            if not sign_stream_cfg['loop']:
                assert ssd.port[ports.y_num].sguid[guid].rx_pkts \
                        == sign_stream_cfg['num_pkts'][i]
                assert ssd.port[ports.y_num].sguid[guid].rx_bytes \
                        == (sign_stream_cfg['num_pkts'][i]
                                * (stream.stream[i].core.frame_len - 4))
        os.remove('capY.pcap')

        # verify tx == rx
        for i in range(sign_stream_cfg['num_streams']):
            guid = sign_stream_cfg['guid'][i]
            if guid < 0:
                continue
            assert ssd.port[ports.x_num].sguid[guid].tx_pkts \
                == ssd.port[ports.y_num].sguid[guid].rx_pkts
            assert ssd.port[ports.x_num].sguid[guid].tx_bytes \
                == ssd.port[ports.y_num].sguid[guid].rx_bytes

            assert ssd.sguid[guid].total.tx_pkts \
                    == ssd.sguid[guid].total.rx_pkts
            assert ssd.sguid[guid].total.pkt_loss == 0

            # for unidir verify rx on tx port is 0 and vice versa
            assert ssd.port[ports.x_num].sguid[guid].rx_pkts == 0
            assert ssd.port[ports.x_num].sguid[guid].rx_bytes == 0
            assert ssd.port[ports.y_num].sguid[guid].tx_pkts == 0
            assert ssd.port[ports.y_num].sguid[guid].tx_bytes == 0

    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(ports.x)

#
# TODO
#  * Verify that bi-directional stream stats are correct for multiple streams
#  * Verify transmit modes
#
