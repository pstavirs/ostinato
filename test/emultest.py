#! /usr/bin/env python

# standard modules
import logging
import os
import subprocess
import sys
import time

from fabric.api import run, env, sudo
from harness import Test, TestSuite

sys.path.insert(1, '../binding')
from core import ost_pb, emul, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac
from protocols.ip4_pb2 import ip4, Ip4

use_defaults = False

# initialize defaults - drone
host_name = '127.0.0.1'
tx_port_number = -1
rx_port_number = -1 
#FIXME:drone_version = ['0', '0', '0']

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
drone = DroneProxy(host_name)

try:
    # ----------------------------------------------------------------- #
    # Baseline Configuration for subsequent testcases
    # ----------------------------------------------------------------- #

    # FIXME: get inputs for dut rx/tx ports

    # configure the DUT
    sudo('sysctl -w net.ipv4.ip_forward=1')
    sudo('ifconfig ' + dut_rx_port + ' 10.10.1.1 netmask 255.255.255.0')
    sudo('ifconfig ' + dut_tx_port + ' 10.10.2.1 netmask 255.255.255.0')

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

    #---------------------------------------------#
    # configure emulated device(s) on tx/rx ports #
    #---------------------------------------------#
    # delete existing devices, if any, on tx port
    did_list = drone.getDeviceIdList(tx_port.port_id[0])
    drone.deleteDevice(did_list)

    # add a emulated device on tx port
    device_id = ost_pb.DeviceIdList()
    device_id.port_id.CopyFrom(tx_port.port_id[0])
    device_id.device_id.add().id = 1
    log.info('adding tx_device %d' % device_id.device_id[0].id)
    drone.addDevice(device_id)

    # configure the device
    device_cfg = ost_pb.DeviceConfigList()
    device_cfg.port_id.CopyFrom(tx_port.port_id[0])
    d = device_cfg.device.add()
    d.device_id.id = device_id.device_id[0].id
    d.core.name = "Host1"
    d.Extensions[emul.mac].addr = 0x000102030001
    ip = d.Extensions[emul.ip4]
    ip.addr = 0x0a0a0164
    ip.prefix_length = 24
    ip.gateway = 0x0a0a0101

    drone.modifyDevice(device_cfg)

    # delete existing devices, if any, on rx port
    did_list = drone.getDeviceIdList(rx_port.port_id[0])
    drone.deleteDevice(did_list)

    # add a emulated device on rx port
    device_id = ost_pb.DeviceIdList()
    device_id.port_id.CopyFrom(rx_port.port_id[0])
    device_id.device_id.add().id = 1
    log.info('adding rx_device %d' % device_id.device_id[0].id)
    drone.addDevice(device_id)

    # configure the device
    device_cfg = ost_pb.DeviceConfigList()
    device_cfg.port_id.CopyFrom(rx_port.port_id[0])
    d = device_cfg.device.add()
    d.device_id.id = device_id.device_id[0].id
    d.core.name = "Host2"
    d.Extensions[emul.mac].addr = 0x000102030002
    ip = d.Extensions[emul.ip4]
    ip.addr = 0x0a0a0264
    ip.prefix_length = 24
    ip.gateway = 0x0a0a0201

    drone.modifyDevice(device_cfg)

    #--------------------------------------#
    # configure traffic stream(s)
    #--------------------------------------#
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
    #s.core.frame_len = 128
    s.control.packets_per_sec = 20
    s.control.num_packets = 100

    # setup stream protocols as mac:eth2:ip4:udp:payload
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x0800278df2b4 #FIXME: hardcoding
    p.Extensions[mac].src_mac = 0x00aabbccddee

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    # reduce typing by creating a shorter reference to p.Extensions[ip4]
    ip = p.Extensions[ip4]
    ip.src_ip = 0x0a0a0164
    ip.dst_ip = 0x0a0a0264

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # all test cases will use this stream by modifying it as per its needs

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    # ----------------------------------------------------------------- #
    # TESTCASE: FIXME
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('startTransmitDuringTransmitIsNopNotRestart')
    # clear arp on DUT
    sudo('arp -d ' + '10.10.1.100', warn_only=True)
    sudo('arp -d ' + '10.10.2.100', warn_only=True)
    run('arp -a')
    drone.startCapture(rx_port)
    drone.startTransmit(tx_port)
    try:
        log.info('waiting for transmit to finish ...')
        time.sleep(7)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-r', 'capture.pcap'])
        print(cap_pkts)
        if '10.10.2.100' in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        run('arp -a')
        suite.test_end(passed)

    suite.complete()

    # delete streams
    log.info('deleting tx_stream %d' % stream_id.stream_id[0].id)
    drone.deleteStream(stream_id)

    # delete devices
    did_list = drone.getDeviceIdList(tx_port.port_id[0])
    drone.deleteDevice(did_list)
    did_list = drone.getDeviceIdList(rx_port.port_id[0])
    drone.deleteDevice(did_list)

    # bye for now
    drone.disconnect()

except Exception as ex:
    log.exception(ex)

finally:
    suite.report()
    if not suite.passed:
        sys.exit(2);
