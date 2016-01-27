#! /usr/bin/env python

# standard modules
import logging
import os
import subprocess
import sys
import time

sys.path.insert(1, '../binding')
import core
from core import ost_pb, DroneProxy
from rpc import RpcError
from protocols.mac_pb2 import mac
from protocols.ip4_pb2 import ip4, Ip4

class Test:
    pass

class TestSuite:
    def __init__(self):
        self.results = []
        self.total = 0
        self.passed = 0
        self.completed = False

    def test_begin(self, name):
        test = Test()
        test.name = name
        test.passed = False
        self.running = test
        print('-----------------------------------------------------------')
        print('@@TEST: %s' % name)
        print('-----------------------------------------------------------')

    def test_end(self, result):
        if self.running:
            self.running.passed = result
            self.results.append(self.running)
            self.total = self.total + 1
            if result:
                self.passed = self.passed + 1
            self.running = None
            print('@@RESULT: %s' % ('PASS' if result else 'FAIL'))
        else:
            raise Exception('Test end without a test begin')

    def report(self):
        print('===========================================================')
        print('TEST REPORT')
        print('===========================================================')
        for test in self.results:
            print('%s: %d' % (test.name, test.passed))
        print('Passed: %d/%d' % (self.passed, self.total))
        print('Completed: %d' % (self.completed))

    def complete(self):
        self.completed = True

    def passed(self):
        return passed == total and self.completed

# initialize defaults
host_name = '127.0.0.1'
tx_port_number = -1
rx_port_number = -1 
drone_version = ['0', '0', '0']

if sys.platform == 'win32':
    tshark = r'C:\Program Files\Wireshark\tshark.exe'
else:
    tshark = 'tshark'
    

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
    # TESTCASE: Verify any RPC before checkVersion() fails and the server
    #           closes the connection
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('anyRpcBeforeCheckVersionFails')
    drone.channel.connect(drone.host, drone.port)
    try:
        port_id_list = drone.getPortIdList()
    except RpcError as e:
        if ('compatibility check pending' in str(e)):
            passed = True
        else:
            raise
    finally:
        drone.channel.disconnect()
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify DroneProxy.connect() fails for incompatible version
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('connectFailsForIncompatibleVersion')
    try:
        orig_version = core.__version__
        core.__version__ = '0.1.1'
        drone.connect()
    except RpcError as e:
        if ('needs client version' in str(e)):
            passed = True
            drone_version = str(e).split()[-1].split('.')
        else:
            raise
    finally:
        core.__version__ = orig_version
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify checkVersion() fails for invalid client version format
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('checkVersionFailsForInvalidClientVersion')
    try:
        orig_version = core.__version__
        core.__version__ = '0-1-1'
        drone.connect()
    except RpcError as e:
        if ('invalid version' in str(e)):
            passed = True
        else:
            raise
    finally:
        core.__version__ = orig_version
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify checkVersion() returns incompatible if the 'major'
    #           part of the <major.minor.revision> numbering format is
    #           different than the server's version and the server closes
    #           the connection
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('checkVersionReturnsIncompatForDifferentMajorVersion')
    try:
        orig_version = core.__version__
        core.__version__ = (str(int(drone_version[0])+1)
                                + '.' + drone_version[1])
        drone.connect()
    except RpcError as e:
        #FIXME: How to check for a closed connection?
        if ('needs client version' in str(e)):
            passed = True
        else:
            raise
    finally:
        core.__version__ = orig_version
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify checkVersion() returns incompatible if the 'minor'
    #           part of the <major.minor.revision> numbering format is
    #           different than the server's version and the server closes
    #           the connection
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('checkVersionReturnsIncompatForDifferentMinorVersion')
    try:
        orig_version = core.__version__
        core.__version__ = (drone_version[0]
                                + '.' + str(int(drone_version[1])+1))
        drone.connect()
    except RpcError as e:
        #FIXME: How to check for a closed connection?
        if ('needs client version' in str(e)):
            passed = True
        else:
            raise
    finally:
        core.__version__ = orig_version
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify checkVersion() returns compatible if the 'revision'
    #           part of the <major.minor.revision> numbering format is
    #           different than the server's version
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('checkVersionReturnsCompatForDifferentRevisionVersion')
    try:
        orig_version = core.__version__
        core.__version__ = (drone_version[0]
                                + '.' + drone_version[1]
                                + '.' + '999')
        drone.connect()
        passed = True
    except RpcError as e:
        raise
    finally:
        drone.disconnect()
        core.__version__ = orig_version
        suite.test_end(passed)

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
    s.control.num_packets = 10

    # setup stream protocols as mac:eth2:ip4:udp:payload
    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    p.Extensions[mac].dst_mac = 0x001122334455
    p.Extensions[mac].src_mac = 0x00aabbccddee

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

    p = s.protocol.add()
    p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber
    # reduce typing by creating a shorter reference to p.Extensions[ip4]
    ip = p.Extensions[ip4]
    ip.src_ip = 0x01020304
    ip.dst_ip = 0x05060708
    ip.dst_ip_mode = Ip4.e_im_inc_host

    s.protocol.add().protocol_id.id = ost_pb.Protocol.kUdpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

    log.info('configuring tx_stream %d' % stream_id.stream_id[0].id)
    drone.modifyStream(stream_cfg)

    # clear tx/rx stats
    log.info('clearing tx/rx stats')
    drone.clearStats(tx_port)
    drone.clearStats(rx_port)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify a RPC with missing required fields in request fails
    #           and subsequently passes when the fields are initialized
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('rpcWithMissingRequiredFieldsFails')
    pid = ost_pb.PortId()
    try:
        sid_list = drone.getStreamIdList(pid)
    except RpcError as e:
        if ('missing required fields in request' in str(e)):
            passed = True
            log.info("Retrying RPC after adding the missing fields")
            pid.id = tx_port_number
            try:
                sid_list = drone.getStreamIdList(pid)
            except:
                passed = False
                raise
        else:
            raise
    finally:
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify invoking addStream() during transmit fails
    # TESTCASE: Verify invoking modifyStream() during transmit fails
    # TESTCASE: Verify invoking deleteStream() during transmit fails
    # ----------------------------------------------------------------- #
    sid = ost_pb.StreamIdList()
    sid.port_id.CopyFrom(tx_port.port_id[0])
    sid.stream_id.add().id = 2

    passed = False
    suite.test_begin('addStreamDuringTransmitFails')
    drone.startTransmit(tx_port)
    try:
        log.info('adding tx_stream %d' % sid.stream_id[0].id)
        drone.addStream(sid)
    except RpcError as e:
        if ('Port Busy' in str(e)):
            passed = True
        else:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    passed = False
    suite.test_begin('modifyStreamDuringTransmitFails')
    scfg = ost_pb.StreamConfigList()
    scfg.port_id.CopyFrom(tx_port.port_id[0])
    s = scfg.stream.add()
    s.stream_id.id = sid.stream_id[0].id
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kMacFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kArpFieldNumber
    s.protocol.add().protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber
    drone.startTransmit(tx_port)
    try:
        log.info('configuring tx_stream %d' % sid.stream_id[0].id)
        drone.modifyStream(scfg)
    except RpcError as e:
        if ('Port Busy' in str(e)):
            passed = True
        else:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    passed = False
    suite.test_begin('deleteStreamDuringTransmitFails')
    drone.startTransmit(tx_port)
    try:
        log.info('deleting tx_stream %d' % sid.stream_id[0].id)
        drone.deleteStream(sid)
    except RpcError as e:
        if ('Port Busy' in str(e)):
            passed = True
        else:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify invoking startTransmit() during transmit is a NOP, 
    #           not a restart
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('startTransmitDuringTransmitIsNopNotRestart')
    drone.startCapture(rx_port)
    drone.startTransmit(tx_port)
    try:
        log.info('sleeping for 4s ...')
        time.sleep(4)
        log.info('starting transmit multiple times')
        drone.startTransmit(tx_port)
        time.sleep(1)
        drone.startTransmit(tx_port)
        time.sleep(1)
        drone.startTransmit(tx_port)
        time.sleep(1)
        log.info('waiting for transmit to finish ...')
        time.sleep(5)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-r', 'capture.pcap'])
        print(cap_pkts)
        if '5.6.7.8' in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify invoking startCapture() during capture is a NOP, 
    #           not a restart
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('startCaptureDuringTransmitIsNopNotRestart')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('sleeping for 4s ...')
        time.sleep(4)
        log.info('starting capture multiple times')
        drone.startCapture(rx_port)
        time.sleep(1)
        drone.startCapture(rx_port)
        time.sleep(1)
        drone.startCapture(rx_port)
        time.sleep(1)
        log.info('waiting for transmit to finish ...')
        time.sleep(5)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-r', 'capture.pcap'])
        print(cap_pkts)
        if '5.6.7.8' in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify invoking stopTransmit() when transmit is not running
    #           is a NOP 
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('stopTransmitWhenTransmitNotRunningIsNop')
    try:
        tx_stats = drone.getStats(tx_port)
        log.info('--> (tx_stats)' + tx_stats.__str__())
        if tx_stats.port_stats[0].state.is_transmit_on:
            raise Exception('Unexpected transmit ON state')
        log.info('stopping transmit multiple times')
        drone.stopTransmit(tx_port)
        time.sleep(1)
        drone.stopTransmit(tx_port)
        time.sleep(1)
        drone.stopTransmit(tx_port)

        # if we reached here, that means there was no exception
        passed = True
    except RpcError as e:
            raise
    finally:
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify invoking stopCapture() when capture is not running 
    #           is a NOP 
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('stopCaptureWhenCaptureNotRunningIsNop')
    try:
        rx_stats = drone.getStats(rx_port)
        log.info('--> (rx_stats)' + rx_stats.__str__())
        if rx_stats.port_stats[0].state.is_capture_on:
            raise Exception('Unexpected capture ON state')
        log.info('stopping capture multiple times')
        drone.stopCapture(rx_port)
        time.sleep(1)
        drone.stopCapture(rx_port)
        time.sleep(1)
        drone.stopCapture(rx_port)

        # if we reached here, that means there was no exception
        passed = True
    except RpcError as e:
            raise
    finally:
        suite.test_end(passed)

    # ----------------------------------------------------------------- #
    # TESTCASE: Verify startCapture(), startTransmit() sequence captures the
    #           first packet
    # TESTCASE: Verify stopTransmit(), stopCapture() sequence captures the
    #           last packet
    # ----------------------------------------------------------------- #
    passed = False
    suite.test_begin('startStopTransmitCaptureOrderCapturesAllPackets')
    try:
        drone.startCapture(rx_port)
        drone.startTransmit(tx_port)
        log.info('waiting for transmit to finish ...')
        time.sleep(12)
        drone.stopTransmit(tx_port)
        drone.stopCapture(rx_port)

        log.info('getting Rx capture buffer')
        buff = drone.getCaptureBuffer(rx_port.port_id[0])
        drone.saveCaptureBuffer(buff, 'capture.pcap')
        log.info('dumping Rx capture buffer')
        cap_pkts = subprocess.check_output([tshark, '-r', 'capture.pcap'])
        print(cap_pkts)
        if '5.6.7.8' in cap_pkts and '5.6.7.17' in cap_pkts:
            passed = True
        os.remove('capture.pcap')
    except RpcError as e:
            raise
    finally:
        drone.stopTransmit(tx_port)
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
