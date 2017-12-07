#! /usr/bin/env python

# standard modules
import logging
import os
import pytest
import subprocess
import sys
import time

from harness import extract_column
from utils import get_tshark

sys.path.insert(1, '../binding')
from core import ost_pb, DroneProxy
from rpc import RpcError
#from protocols.mac_pb2 import mac
#from protocols.payload_pb2 import payload, Payload

# initialize defaults
host_name = '127.0.0.1'
tx_number = -1
rx_number = -1

tshark = get_tshark(minversion='1.2.0') 

fmt = 'column.format:"Packet#","%m","Time","%t","Source","%uns","Destination","%und","Protocol","%p","Size","%L","Info","%i","Expert","%a"'
fmt_col = 7 # col# in fmt for Size/PktLength

# setup protocol number dictionary
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

print('')
print('This test uses the following topology -')
print('')
print(' +-------+           ')
print(' |       |Tx--->----+')
print(' | Drone |          |')
print(' |       |Rx---<----+')
print(' +-------+           ')
print('')
print('A loopback port is used as both the Tx and Rx ports')
print('')

@pytest.fixture(scope='module')
def drone(request):
    """Baseline Configuration for all testcases in this module"""

    dut = DroneProxy(host_name)

    log.info('connecting to drone(%s:%d)' % (dut.hostName(), dut.portNumber()))
    dut.connect()

    def fin():
        dut.disconnect()

    request.addfinalizer(fin)

    return dut

@pytest.fixture(scope='module')
def ports(request, drone):
    # retreive port id list
    log.info('retreiving port list')
    port_id_list = drone.getPortIdList()

    # retreive port config list
    log.info('retreiving port config for all ports')
    port_config_list = drone.getPortConfig(port_id_list)

    if len(port_config_list.port) == 0:
        log.warning('drone has no ports!')
        sys.exit(1)

    # iterate port list to find a loopback port to use as the tx/rx port id
    print('Port List')
    print('---------')
    for port in port_config_list.port:
        print('%d.%s (%s)' % (port.port_id.id, port.name, port.description))
        # use a loopback port as default tx/rx port
        if ('lo' in port.name or 'loopback' in port.description.lower()):
            tx_number = port.port_id.id
            rx_number = port.port_id.id

    if tx_number < 0 or rx_number < 0:
        log.warning('loopback port not found')
        sys.exit(1)

    print('Using port %d as tx/rx port(s)' % tx_number)

    ports.tx = ost_pb.PortIdList()
    ports.tx.port_id.add().id = tx_number;

    ports.rx = ost_pb.PortIdList()
    ports.rx.port_id.add().id = rx_number;

    # stop transmit/capture on ports, if any
    drone.stopTransmit(ports.tx)
    drone.stopCapture(ports.rx)

    # delete existing streams, if any, on tx port
    sid_list = drone.getStreamIdList(ports.tx.port_id[0])
    drone.deleteStream(sid_list)

    return ports

@pytest.fixture(scope='module')
def stream(request, drone, ports):
    global proto_number

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.tx.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    # configure the stream
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.tx.port_id[0])
    s = stream_cfg.stream.add()
    s.stream_id.id = stream_id.stream_id[0].id
    s.core.is_enabled = True
    s.control.packets_per_sec = 100
    s.control.num_packets = 10

    # XXX: don't setup any stream protocols

    def fin():
        # delete streams
        log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
        drone.deleteStream(stream_id)

    request.addfinalizer(fin)

    return stream_cfg

protolist=['mac eth2 ip4 udp payload', 'mac eth2 ip4 udp']
@pytest.fixture(scope='module', params=protolist)
def stream_toggle_payload(request, drone, ports):
    global proto_number

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(ports.tx.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    # configure the stream
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(ports.tx.port_id[0])
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

@pytest.mark.parametrize("mode", [
    ost_pb.StreamCore.e_fl_inc,
    ost_pb.StreamCore.e_fl_dec,
    ost_pb.StreamCore.e_fl_random
])
def test_framelen_modes(drone, ports, stream_toggle_payload, mode):
    """ Test various framelen modes """

    min_pkt_len = 100
    max_pkt_len = 1000
    stream = stream_toggle_payload
    stream.stream[0].core.len_mode = mode
    stream.stream[0].core.frame_len_min = min_pkt_len
    stream.stream[0].core.frame_len_max = max_pkt_len

    log.info('configuring tx_stream %d' % stream.stream[0].stream_id.id)
    drone.modifyStream(stream)

    if stream.stream[0].protocol[-1].protocol_id.id \
            == ost_pb.Protocol.kPayloadFieldNumber:
       filter = 'udp && !expert.severity'
    else:
       filter = 'udp && eth.fcs_bad==1' \
                '&& ip.checksum_bad==0 && udp.checksum_bad==0'

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(ports.tx)
    drone.clearStats(ports.rx)

    try:
        drone.startCapture(ports.rx)
        drone.startTransmit(ports.tx)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(ports.tx)
        drone.stopCapture(ports.rx)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(ports.rx.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap'])
        print(cap_pkts)
        print(filter)
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-Y', filter, '-o', fmt])
        print(cap_pkts)
        assert cap_pkts.count('\n') == stream.stream[0].control.num_packets
        result = extract_column(cap_pkts, fmt_col)
        print(result)
        diffSum = 0
        for i in range(len(result)):
            l = int(result[i]) + 4 # add FCS to length
            assert (l >= min_pkt_len) and (l <= max_pkt_len)

            # check current packet length to last
            if (i > 0):
                ll = int(result[i-1]) + 4
                if mode == ost_pb.StreamCore.e_fl_inc:
                    assert l == (ll+1)
                elif mode == ost_pb.StreamCore.e_fl_dec:
                    assert l == (ll-1)
                elif mode == ost_pb.StreamCore.e_fl_random:
                    diffSum += (l-ll)

        # TODO: find a better way to check for randomness
        if mode == ost_pb.StreamCore.e_fl_random:
            assert (diffSum % (len(result) - 1)) != 0

        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(ports.tx)

def test_frame_trunc(drone, ports, stream):
    """ Test frame is truncated even if the protocol sizes exceed framelen """

    #stream.stream[0].core.frame_len_min = min_pkt_len

    # setup stream protocols as mac:vlan:eth2:ip6:udp:payload
    stream.stream[0].ClearField("protocol")
    protos = ['mac', 'vlan', 'eth2', 'ip6', 'udp', 'payload']
    for p in protos:
        stream.stream[0].protocol.add().protocol_id.id = proto_number[p]

    # note: unless truncated minimum size of payload protocol is 1
    exp_framelen = min(12+4+2+40+8+1, 60)

    log.info('configuring tx_stream %d' % stream.stream[0].stream_id.id)
    drone.modifyStream(stream)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(ports.tx)
    drone.clearStats(ports.rx)

    try:
        drone.startCapture(ports.rx)
        drone.startTransmit(ports.tx)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(ports.tx)
        drone.stopCapture(ports.rx)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(ports.rx.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-Y', 'vlan', '-o', fmt])
        print(cap_pkts)
        assert cap_pkts.count('\n') == stream.stream[0].control.num_packets
        result = extract_column(cap_pkts, fmt_col)
        print(result)
        for i in range(len(result)):
            assert (int(result[i]) == exp_framelen)

        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(ports.tx)
