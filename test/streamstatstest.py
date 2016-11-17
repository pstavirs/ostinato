#! /usr/bin/env python

# standard modules
import ipaddress
import logging
import os
import pytest
import random
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

tshark = get_tshark(minversion = '1.2.0') # FIXME: do we need a minversion?

# setup protocol number dictionary
# FIXME: remove if not reqd.
proto_number = {}
proto_number['mac'] = ost_pb.Protocol.kMacFieldNumber
proto_number['vlan'] = ost_pb.Protocol.kVlanFieldNumber
proto_number['eth2'] = ost_pb.Protocol.kEth2FieldNumber
proto_number['ip4'] = ost_pb.Protocol.kIp4FieldNumber
proto_number['ip6'] = ost_pb.Protocol.kIp6FieldNumber
proto_number['udp'] = ost_pb.Protocol.kUdpFieldNumber
proto_number['payload'] = ost_pb.Protocol.kPayloadFieldNumber

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

print(' +-------+           +-------+')
print(' |       |X--<-->--X|-+     |')
print(' | Drone |          | | DUT |')
print(' |       |Y--<-->--Y|-+     |')
print(' +-------+           +-------+')
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
        if ('vhost' in port.name or 'sun' in port.description.lower()):
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
    portConfig.port[0].track_stream_stats = True;
    portConfig.port.add().port_id.id = ports.y_num;
    portConfig.port[1].track_stream_stats = True;
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

@pytest.fixture
def stream(request, drone, ports, sign_stream_cfg):

    # add stream(s)
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.x.port_id[0])
    stream_id.stream_id.add().id = 1  # Unsigned stream
    stream_id.stream_id.add().id = 2  # Signed stream
    stream_id.stream_id.add().id = 3  # Signed stream
    log.info('adding X stream(s) ' + str(stream_id))
    drone.addStream(stream_id)

    # configure the stream(s)
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.x.port_id[0])
    s = stream_cfg.stream.add()
    s.stream_id.id = stream_id.stream_id[0].id
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

    # setup (unsigned) stream protocols as mac:eth2:ip:payload
    p = s.protocol.add()
    s.control.num_packets = 10
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac_mode = Mac.e_mm_resolve
    p.Extensions[mac].src_mac_mode = Mac.e_mm_resolve

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x0a0a0165
    p.Extensions[ip4].dst_ip = 0x0a0a0265

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    # setup (signed) stream protocols as mac:eth2:ip:udp|tcp:payload
    # Remove payload, add udp, payload and sign protocol to signed stream(s)
    sscfg = sign_stream_cfg
    s = stream_cfg.stream.add()
    s.CopyFrom(stream_cfg.stream[0])
    s.stream_id.id = stream_id.stream_id[1].id
    s.control.num_packets = sscfg['num_pkts'][0]
    if sscfg['num_var_pkts'][0]:
        s.protocol[2].Extensions[ip4].src_ip_mode = Ip4.e_im_inc_host
        s.protocol[2].Extensions[ip4].src_ip_count = sscfg['num_var_pkts'][0]
    del s.protocol[-1]
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kSignFieldNumber
    p.Extensions[sign].stream_guid = 101

    s = stream_cfg.stream.add()
    s.CopyFrom(stream_cfg.stream[1])
    s.stream_id.id = stream_id.stream_id[2].id
    s.core.frame_len = 128
    s.control.num_packets = sscfg['num_pkts'][1]
    if sscfg['num_var_pkts'][1]:
        s.protocol[2].Extensions[ip4].src_ip_mode = Ip4.e_im_inc_host
        s.protocol[2].Extensions[ip4].src_ip_count = sscfg['num_var_pkts'][1]
    s.protocol[-3].protocol_id.id = ost_pb.Protocol.kTcpFieldNumber
    s.protocol[-1].Extensions[sign].stream_guid = 102

    if sscfg['loop']:
        s.control.next = ost_pb.StreamControl.e_nw_goto_id

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

"""
# FIXME: remove if not required
protolist=['mac eth2 ip4 udp payload', 'mac eth2 ip4 udp']
@pytest.fixture(scope='module', params=protolist)
def stream_toggle_payload(request, drone, ports):
    global proto_number

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.x.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    # configure the stream
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.x.port_id[0])
    s = stream_cfg.stream.add()
    s.stream_id.id = stream_id.stream_id[0].id
    s.core.is_enabled = True
    s.control.packets_per_sec = 100
    s.control.num_packets = 10

    # setup stream protocols
    s.ClearField("protocol")
    protos = request.param.split()
    for p in protos:
        s.protocol.add().protocol_id.id = proto_number[p]

    def fin():
        # delete streams
        log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
        drone.deleteStream(stream_id)

    request.addfinalizer(fin)

    return stream_cfg
"""

# ================================================================= #
# ----------------------------------------------------------------- #
#                            TEST CASES
# ----------------------------------------------------------------- #
# ================================================================= #

@pytest.mark.parametrize('sign_stream_cfg', [
    {'num_pkts': [270, 512], 'num_var_pkts': [0, 0], 'loop': False},
    {'num_pkts': [276, 510], 'num_var_pkts': [0, 0], 'loop': True},
    #{'num_pkts': [19, 30], 'num_var_pkts': [0, 0], 'loop': True},
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
               /.101         \.101
            HostX           HostY
     """

    # calculate tx pkts
    total_tx_pkts = 10 + sign_stream_cfg['num_pkts'][0] \
                       + sign_stream_cfg['num_pkts'][1]
    # clear port X/Y stats
    log.info('clearing stats')
    drone.clearStats(ports.x)
    drone.clearStats(ports.y)
    drone.clearStreamStats(stream_guids)

    # verify stream strats are indeed cleared
    ssd = drone.getStreamStatsDict(stream_guids)
    assert len(ssd.port) == 0

    # resolve ARP/NDP on ports X/Y
    log.info('resolving Neighbors on (X, Y) ports ...')
    drone.resolveDeviceNeighbors(emul_ports)
    time.sleep(3)

    # FIXME: dump ARP/NDP table on devices and DUT

    try:
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
        # FIXME (temp): assert len(ssd.port) == 2

        # dump X capture buffer
        log.info('getting X capture buffer')
        buff = drone.getCaptureBuffer(ports.x.port_id[0])
        drone.saveCaptureBuffer(buff, 'capX.pcap')
        log.info('dumping X capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capX.pcap'])
        print(cap_pkts)

        # get sign stream #1 Tx stats from capture
        filter="frame[-9:9]==00.00.00.65.61.a1.b2.c3.d4"
        print(filter)
        log.info('dumping X capture buffer (filtered)')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capX.pcap',
            '-Y', filter])
        print(cap_pkts)
        sign_stream1_cnt = cap_pkts.count('\n')

        # get sign stream #2 Tx stats from capture
        filter="frame[-9:9]==00.00.00.66.61.a1.b2.c3.d4"
        print(filter)
        log.info('dumping Y capture buffer (filtered)')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capX.pcap',
            '-Y', filter])
        print(cap_pkts)
        sign_stream2_cnt = cap_pkts.count('\n')

        os.remove('capX.pcap')

        log.info('sign stream 101 tx cap count: %d' % (sign_stream1_cnt))
        log.info('sign stream 102 tx cap count: %d' % (sign_stream2_cnt))
        log.info('--> (stream stats)\n' + str(ssd))

        # verify tx stream stats from drone is same as that from capture
        assert len(ssd.port[ports.x_num].sguid) == 2
        if sign_stream_cfg['loop']:
            assert ssd.port[ports.x_num].sguid[101].tx_pkts \
                    == sign_stream1_cnt
            assert ssd.port[ports.x_num].sguid[101].tx_bytes \
                    == (sign_stream1_cnt
                            * (stream.stream[1].core.frame_len - 4))
            assert ssd.port[ports.x_num].sguid[102].tx_pkts \
                    == sign_stream2_cnt
            assert ssd.port[ports.x_num].sguid[102].tx_bytes \
                    == (sign_stream2_cnt
                            * (stream.stream[2].core.frame_len - 4))
        else:
            assert ssd.port[ports.x_num].sguid[101].tx_pkts \
                    == sign_stream_cfg['num_pkts'][0]
            assert ssd.port[ports.x_num].sguid[101].tx_bytes \
                    == (sign_stream_cfg['num_pkts'][0]
                            * (stream.stream[1].core.frame_len - 4))
            assert ssd.port[ports.x_num].sguid[102].tx_pkts \
                    == sign_stream_cfg['num_pkts'][1]
            assert ssd.port[ports.x_num].sguid[102].tx_bytes \
                    == (sign_stream_cfg['num_pkts'][1]
                            * (stream.stream[2].core.frame_len - 4))

        # dump Y capture buffer
        log.info('getting Y capture buffer')
        buff = drone.getCaptureBuffer(ports.y.port_id[0])
        drone.saveCaptureBuffer(buff, 'capY.pcap')
        log.info('dumping Y capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capY.pcap'])
        print(cap_pkts)

        # get sign stream #1 Rx stats from capture
        filter="frame[-9:9]==00.00.00.65.61.a1.b2.c3.d4"
        print(filter)
        log.info('dumping Y capture buffer (filtered)')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capY.pcap',
            '-Y', filter])
        print(cap_pkts)
        sign_stream1_cnt = cap_pkts.count('\n')

        # get sign stream #2 Rx stats from capture
        filter="frame[-9:9]==00.00.00.66.61.a1.b2.c3.d4"
        print(filter)
        log.info('dumping Y capture buffer (filtered)')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capY.pcap',
            '-Y', filter])
        print(cap_pkts)
        sign_stream2_cnt = cap_pkts.count('\n')

        os.remove('capY.pcap')

        log.info('--> (x_stats)' + x_stats.__str__())
        log.info('--> (y_stats)' + y_stats.__str__())
        log.info('sign stream 101 rx cap count: %d' % (sign_stream1_cnt))
        log.info('sign stream 102 rx cap count: %d' % (sign_stream2_cnt))
        log.info('--> (stream stats)\n' + str(ssd))

        # verify rx stream stats from drone is same as that from capture
        assert len(ssd.port[ports.y_num].sguid) == 2
        if sign_stream_cfg['loop']:
            assert ssd.port[ports.y_num].sguid[101].rx_pkts \
                    == sign_stream1_cnt
            assert ssd.port[ports.y_num].sguid[101].rx_bytes \
                    == (sign_stream1_cnt
                            * (stream.stream[1].core.frame_len - 4))
            assert ssd.port[ports.y_num].sguid[102].rx_pkts \
                    == sign_stream2_cnt
            assert ssd.port[ports.y_num].sguid[102].rx_bytes \
                    == (sign_stream2_cnt
                            * (stream.stream[2].core.frame_len - 4))
        else:
            assert ssd.port[ports.y_num].sguid[101].rx_pkts \
                    == sign_stream_cfg['num_pkts'][0]
            assert ssd.port[ports.y_num].sguid[101].rx_bytes \
                    == (sign_stream_cfg['num_pkts'][0]
                            * (stream.stream[1].core.frame_len - 4))
            assert ssd.port[ports.y_num].sguid[102].rx_pkts \
                    == sign_stream_cfg['num_pkts'][1]
            assert ssd.port[ports.y_num].sguid[102].rx_bytes \
                    == (sign_stream_cfg['num_pkts'][1]
                            * (stream.stream[2].core.frame_len - 4))

        # verify tx == rx
        assert ssd.port[ports.x_num].sguid[101].tx_pkts \
            == ssd.port[ports.y_num].sguid[101].rx_pkts
        assert ssd.port[ports.x_num].sguid[101].tx_bytes \
            == ssd.port[ports.y_num].sguid[101].rx_bytes
        assert ssd.port[ports.x_num].sguid[102].tx_pkts \
            == ssd.port[ports.y_num].sguid[102].rx_pkts
        assert ssd.port[ports.x_num].sguid[102].tx_bytes \
            == ssd.port[ports.y_num].sguid[102].rx_bytes

        # for unidir verify rx on tx port is 0 and vice versa
        # FIXME: failing currently because tx pkts on tx port seem to be
        # captured by rxStatsPoller_ on tx port
        """
        assert ssd.port[ports.x_num].sguid[101].rx_pkts == 0
        assert ssd.port[ports.x_num].sguid[101].rx_bytes == 0
        assert ssd.port[ports.y_num].sguid[101].tx_pkts == 0
        assert ssd.port[ports.y_num].sguid[101].tx_bytes == 0
        """

    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(ports.x)

#
# TODO
#  * Verify that uni-directional stream stats are correct for a single stream
#  * Verify that uni-directional stream stats are correct for multiple streams
#  * Verify that bi-directional stream stats are correct for a single stream
#  * Verify that bi-directional stream stats are correct for multiple streams
#  * Verify protocol combinations - Eth, IPv4/IPv6, TCP/UDP, Pattern
#  * Verify transmit modes
#  * Verify tx pkts = multiple repeats of packetList + partial repeat
#  * Verify tx pkts = multiple complete repeats of packetList + no partials
#  * Verify tx pkts = first seq partial
#
