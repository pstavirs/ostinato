#! /usr/bin/env python

# standard modules
import logging
import os
import subprocess
import sys
import time

from harness import Test, TestSuite, extract_column

sys.path.insert(1, '../binding')
from core import ost_pb, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac
from protocols.arp_pb2 import arp
from protocols.icmp_pb2 import icmp
from protocols.ip4_pb2 import ip4, Ip4
from protocols.ip6_pb2 import ip6, Ip6
from protocols.payload_pb2 import payload, Payload

# initialize defaults
host_name = '127.0.0.1'
tx_port_number = -1
rx_port_number = -1 
drone_version = ['0', '0', '0']

if sys.platform == 'win32':
    tshark = r'C:\Program Files\Wireshark\tshark.exe'
else:
    tshark = 'tshark'
fmt_template = 'column.format:"Packet#","%m","Time","%t","Source","%uns","Destination","%und","Protocol","%p","Size","%L",<custom>,"Info","%i","Expert","%a"'
fmt = fmt_template.replace('<custom>', '"Custom","%Cus:XXX"')
fmt2 = fmt_template.replace('<custom>', '"Custom","%Cus:XXX",'*2).replace(',,',',')
fmt4 = fmt_template.replace('<custom>', '"Custom","%Cus:XXX",'*4).replace(',,',',')
fmt_col = 8 # col# in fmt for Custom (col numbers start from 1)

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

suite = TestSuite()
drone = DroneProxy(host_name)

try:
    # ----------------------------------------------------------------- #
    # Baseline Configuration for subsequent testcases
    # ----------------------------------------------------------------- #

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

    # iterate port list to find a loopback port to use as the tx/rx port id 
    print('Port List')
    print('---------')
    for port in port_config_list.port:
        print('%d.%s (%s)' % (port.port_id.id, port.name, port.description))
        # use a loopback port as default tx/rx port 
        if ('lo' in port.name or 'loopback' in port.description.lower()):
            tx_port_number = port.port_id.id
            rx_port_number = port.port_id.id

    if tx_port_number < 0 or rx_port_number < 0:
        log.warning('loopback port not found')
        sys.exit(1)

    print('Using port %d as tx/rx port(s)' % tx_port_number)

    tx_port = ost_pb.PortIdList()
    tx_port.port_id.add().id = tx_port_number;

    rx_port = ost_pb.PortIdList()
    rx_port.port_id.add().id = rx_port_number;

    # delete existing streams, if any, on tx port
    sid_list = drone.getStreamIdList(tx_port.port_id[0])
    drone.deleteStream(sid_list)

    # add a stream
    stream_id = ost_pb.StreamIdList()
    stream_id.port_id.CopyFrom(tx_port.port_id[0])
    stream_id.stream_id.add().id = 1
    log.info('adding tx_stream %d' % stream_id.stream_id[0].id)
    drone.addStream(stream_id)

    # configure the stream
    stream_cfg = ost_pb.StreamConfigList()
    stream_cfg.port_id.CopyFrom(tx_port.port_id[0])
    s = stream_cfg.stream.add()
    s.stream_id.id = stream_id.stream_id[0].id
    s.core.is_enabled = True
    s.core.frame_len = 128
    s.control.packets_per_sec = 100
    s.control.num_packets = 10

    # all test cases will use this stream by modifying it as per its needs

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify decrement counter8 variable field not affecting
    #           any checksums - IPv6 HopLimit
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip6:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp6FieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter8
    vf.offset = 7
    vf.mask = 0xff
    vf.value = 255
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 4

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter8Decrement[Ip6HopLimit]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt.replace('XXX', 'ipv6.hlim')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected =  ['255', '254', '253', '252', 
                     '255', '254', '253', '252', 
                     '255', '254']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment bitmasked counter8 variable field not 
    #           affecting any checksums - Vlan Prio
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:vlan:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter8
    vf.offset = 2
    vf.mask = 0xe0
    vf.value = 0x60
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 8
    vf.step = 0x10 # step by 1 and repeat each value twice!!!!!

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter8BitmaskIncrement[VlanPrio]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt.replace('XXX', 'vlan.priority')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected =  ['Excellent', 'Excellent', 'Controlled', 'Controlled', 'Video,', 'Video,', 'Voice,', 'Voice,', 'Excellent', 'Excellent']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter16 variable field not affecting
    #           any checksums - vlan-id
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:vlan:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 2
    vf.mask = 0x0fff
    vf.value = 1234
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 7

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter16Increment[vlanid]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt.replace('XXX', 'vlan.id')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['1234', '1235', '1236', '1237', '1238', '1239', '1240', 
                    '1234', '1235', '1236']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify decrement counter32 step 3 variable field not 
    #           affecting any checksums - ARP Target IP
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:arp
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0xffffffffffff

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kArpFieldNumber
    p.Extensions[arp].sender_hw_addr = 0x001122334455
    p.Extensions[arp].sender_proto_addr = 0x01020304
    p.Extensions[arp].target_proto_addr = 0x0a0b0c01
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter32
    vf.offset = 24
    vf.mask = 0x0000ff00
    vf.value = 0x00006400
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 7
    vf.step = 0x00000300

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter32BitmaskDecrement[arpTargetIp]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'arp && frame.len==124',
                        '-o', fmt.replace('XXX', 'arp.dst.proto_ipv4')
                                 .replace('un', 'uh')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['10.11.100.1', '10.11.97.1', '10.11.94.1', '10.11.91.1', 
                    '10.11.88.1',  '10.11.85.1', '10.11.82.1', '10.11.100.1', 
                    '10.11.97.1',  '10.11.94.1']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter8 variable field affecting  
    #           IPv4 checksum - IPv4 TTL
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter8
    vf.offset = 8
    #vf.mask = 0xFF
    vf.value = 127
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 8

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter8IncrementIpCksum[IpTtl]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt.replace('XXX', 'ip.ttl')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['127', '128', '129', '130', '131', '132', '133', '134', 
                    '127', '128']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify decrement counter16 bitmask variable field affecting  
    #           IPv4 checksum - IPv4 FragOfs
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 6
    vf.mask = 0x1FFF
    vf.value = 1000 
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 6

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter16BitmaskDecrementIpCksum[IpFragOfs]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'ip && frame.len==124',
                        '-o', fmt.replace('XXX', 'ip.frag_offset')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['8000', '7992', '7984', '7976', '7968', '7960', 
                    '8000', '7992', '7984', '7976']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter32 bitmask variable field affecting  
    #           IPv4 checksum - IPv4 Dst Addr
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter32
    vf.offset = 16
    vf.mask = 0x00FF0000
    vf.value = 0x00ab0000 
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 3
    vf.step = 0x00020000

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter32BitmaskDecrementIpCksum[IpDstAddr]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt.replace('XXX', 'ip.dst')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['5.171.7.8', '5.173.7.8', '5.175.7.8',
                    '5.171.7.8', '5.173.7.8', '5.175.7.8', 
                    '5.171.7.8', '5.173.7.8', '5.175.7.8', 
                    '5.171.7.8']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter32 variable field affecting  
    #           UDP checksum - seq# in payload
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:udp:payload
    s.ClearField("protocol")
    s.core.frame_len = 64
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter32
    vf.offset = 0
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 100

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter32IncrementUdpCksum[PayloadSeq]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==60',
                        '-o', fmt.replace('XXX', 'data.data')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['000000000000', '000000010000', '000000020000', 
                    '000000030000', '000000040000', '000000050000', 
                    '000000060000', '000000070000', '000000080000', 
                    '000000090000']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        s.core.frame_len = 128 # restore frame length
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter32 variable field affecting  
    #           TCP checksum - TCP seq
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:tcp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kTcpFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter32
    vf.offset = 4
    vf.value = 1000
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 5
    vf.step = 5

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter32IncrementTcpCksum[TcpSeq]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'tcp && frame.len==124',
                        '-o', 'tcp.relative_sequence_numbers:0',
                        '-o', fmt.replace('XXX', 'tcp.seq')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['1000', '1005', '1010', '1015', '1020', 
                    '1000', '1005', '1010', '1015', '1020']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify increment counter16 variable field affecting  
    #           ICMP checksum - ICMP seq
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip4:icmp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIcmpFieldNumber
    p.Extensions[icmp].identifier = 0xabcd
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 6
    vf.value = 1
    vf.count = 10

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
    p.Extensions[payload].pattern_mode = Payload.e_dp_inc_byte

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter16IncrementIcmpCksum[IcmpSeq]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'icmp && frame.len==124',
                        '-o', fmt.replace('XXX', 'icmp.seq')])
        print(cap_pkts)
        result = extract_column(cap_pkts, fmt_col)
        expected = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10']
        log.info('result  : %s' % result)
        log.info('expected: %s' % expected)
        if result == expected and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify xcrement counter16 variable fields affecting  
    #           UDP checksum - UDP src/dst port
    # ----------------------------------------------------------------- #

    # setup stream protocols as mac:eth2:ip6:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp6FieldNumber
    ip = p.Extensions[ip6]
    ip.src_addr_hi = 0x2002000000000000
    ip.src_addr_lo = 0x1001
    ip.dst_addr_hi = 0x4004000000000000
    ip.dst_addr_lo = 0x1001

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kUdpFieldNumber

    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 0
    vf.value = 5000
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 10

    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 2
    vf.value = 6000
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 10

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counter16XcrementUdpCksum[UdpSrcDstPort]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'udp && frame.len==124',
                        '-o', fmt2.replace('XXX', 'udp.srcport', 1)
                                  .replace('XXX', 'udp.dstport')])
        print(cap_pkts)
        result1 = extract_column(cap_pkts, fmt_col)
        expected1 = ['5000', '5001', '5002', '5003', '5004', 
                     '5005', '5006', '5007', '5008', '5009']
        log.info('result1  : %s' % result1)
        log.info('expected1: %s' % expected1)
        result2 = extract_column(cap_pkts, fmt_col+1)
        expected2 = ['6000', '5999', '5998', '5997', '5996', 
                     '5995', '5994', '5993', '5992', '5991']
        log.info('result2  : %s' % result2)
        log.info('expected2: %s' % expected2)
        if result1 == expected1 and result2 == expected2 \
                and 'Error' not in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify multiple Xcrement counterX variable fields affecting  
    #           IP, UDP checksum - VlanId, SrcIp, TcpDstPort, PayloadSeq
    # ----------------------------------------------------------------- #

    s.core.frame_len = 64
    # setup stream protocols as mac:vlan:eth2:ip4:udp:payload
    s.ClearField("protocol")
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 2
    vf.value = 3 
    vf.mask = 0x0FFF
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 10

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    p.Extensions[ip4].src_ip = 0x01020304
    p.Extensions[ip4].dst_ip = 0x05060708
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter32
    vf.offset = 12
    vf.mask = 0x0000ff00
    vf.value = 0x00000100
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 10
    vf.step = 0x00000100

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kTcpFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter16
    vf.offset = 2
    vf.value = 6666
    vf.mode = ost_pb.VariableField.kDecrement
    vf.count = 10

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
    vf = p.variable_field.add()
    vf.type = ost_pb.VariableField.kCounter8
    vf.offset = 0
    vf.mode = ost_pb.VariableField.kIncrement
    vf.count = 10

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    passed = False
    suite.test_begin('counterXcrementIpTcpCksum[Multiple]')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(3)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-n', '-r', 'capture.pcap',
                        '-R', 'tcp && frame.len==60',
                        '-o', fmt4.replace('XXX', 'vlan.id', 1)
                                  .replace('XXX', 'ip.src', 1)
                                  .replace('XXX', 'tcp.dstport', 1)
                                  .replace('XXX', 'data.data', 1)])
        print(cap_pkts)
        result = []
        expected = []
        expected.append(['3', '2', '1', '0', '4095', 
                         '4094', '4093', '4092', '4091', '4090'])
        expected.append(['1.2.1.4', '1.2.2.4', '1.2.3.4', '1.2.4.4', '1.2.5.4',
                         '1.2.6.4', '1.2.7.4', '1.2.8.4', '1.2.9.4', '1.2.10.4'])
        expected.append(['6666', '6665', '6664', '6663', '6662',
                         '6661', '6660', '6659', '6658', '6657'])
        expected.append(['0000', '0100', '0200', '0300', '0400', 
                         '0500', '0600', '0700', '0800', '0900'])
        passed = True
        for i in range(4):
            result.append(extract_column(cap_pkts, fmt_col+i))
            log.info('result[%d]  : %s' % (i, result[i]))
            log.info('expected[%d]: %s' % (i, expected[i]))
            if result[i] != expected[i]:
                passed = False
        if 'Error' in cap_pkts:
            passed = False
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        s.core.frame_len = 128
        suite.test_end(passed)

    suite.complete()

    # delete streams
    log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
    drone.deleteStream(stream_id)

    # bye for now
    drone.disconnect()

except Exception as ex:
    log.exception(ex)

finally:
    suite.report()
    if not suite.passed:
        sys.exit(2);
