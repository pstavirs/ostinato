# Copyright (C) 2014 Srivats P.
# 
# This file is part of "Ostinato"
# 
# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

import os
from rpc import OstinatoRpcChannel, OstinatoRpcController, RpcError
import protocols.protocol_pb2 as ost_pb
import protocols.emulproto_pb2 as emul
from __init__ import __version__

class DroneProxy(object):

    def __init__(self, host_name, port_number=7878):
        self.host = host_name
        self.port = port_number
        self.channel = OstinatoRpcChannel()
        self.stub = ost_pb.OstService_Stub(self.channel)
        self.void = ost_pb.Void()

        for method in self.stub.GetDescriptor().methods:
            fn = lambda request=self.void, method_name=method.name: \
                self.callRpcMethod(method_name, request)
            self.__dict__[method.name] = fn

    def hostName(self):
        return self.host

    def portNumber(self):
        return self.port

    def connect(self):
        self.channel.connect(self.host, self.port)
        ver = ost_pb.VersionInfo()
        ver.client_name = 'python-ostinato'
        ver.version = __version__
        compat = self.checkVersion(ver)
        if compat.result == ost_pb.VersionCompatibility.kIncompatible:
            raise RpcError('incompatible version %s (%s)' % 
                    (ver.version, compat.notes))

    def disconnect(self):
        self.channel.disconnect()

    def callRpcMethod(self, method_name, request):
        controller = OstinatoRpcController()
        ost_pb.OstService_Stub.__dict__[method_name](
                    self.stub, controller, request, None)
        return controller.response

    def saveCaptureBuffer(self, buffer, file_name):
         f= open(file_name, 'wb')
         f.write(buffer)
         f.flush()
         os.fsync(f.fileno())
         f.close()

    def getStreamStatsDict(self, stream_guid_list): # FIXME: rename?
        """
        Convenience method for fetching stream stats which returns an object
        containing port/sguid dictionaries for easier access e.g.

        stream_stats = drone.getStreamStatsDict(guid_list)

        stream_stats.sguid[101].port[1].tx_pkts
        stream_stats.port[1].sguid[101].rx_bytes

        In addition, you can also retrieve totals across ports, e.g.

        stream_stats.port[1].total.rx_pkts
        stream_stats.port[1].total.rx_bytes
        stream_stats.port[1].total.tx_pkts
        stream_stats.port[1].total.tx_bytes

        and totals across sguids -

        stream_stats.sguid[101].total.tx_pkts
        stream_stats.sguid[101].total.rx_pkts
        stream_stats.sguid[101].total.pkt_loss

        This method is a wrapper around the getStreamStats() RPC
        """
        class StreamStatsPortTotal:
            def __init__(self):
                self.tx_pkts = 0
                self.rx_pkts = 0
                self.tx_bytes = 0
                self.rx_bytes = 0
            def __repr__(self):
                s  = '  total: { \n'
                s += '    rx_pkts: ' + str(self.rx_pkts) + ' \n'
                s += '    rx_bytes: ' + str(self.rx_bytes) + ' \n'
                s += '    tx_pkts: ' + str(self.tx_pkts) + ' \n'
                s += '    tx_bytes: ' + str(self.tx_bytes) + ' \n'
                s += '  }\n'
                return s
        class StreamStatsDictPort:
            def __init__(self):
                self.sguid = dict()
                self.total = StreamStatsPortTotal()
            def __repr__(self):
                s = '  sguid: { \n'
                for k, v in self.sguid.items():
                    s += '    ' + str(k) + ': {\n      ' \
                            + str(v).replace('\n', '\n      ') + '}\n'
                s += '  }\n'
                s += str(self.total)
                return s
        class StreamStatsGuidTotal:
            def __init__(self):
                self.tx_pkts = 0
                self.rx_pkts = 0
                self.pkt_loss = 0
            def __repr__(self):
                s  = '  total: { \n'
                s += '    tx_pkts: ' + str(self.tx_pkts) + ' \n'
                s += '    rx_pkts: ' + str(self.rx_pkts) + ' \n'
                s += '    pkt_loss: ' + str(self.pkt_loss) + ' \n'
                s += '  }\n'
                return s
        class StreamStatsDictGuid:
            def __init__(self):
                self.port = dict()
                self.total = StreamStatsGuidTotal()
            def __repr__(self):
                s = '  port: { \n'
                for k, v in self.port.items():
                    s += '    ' + str(k) + ': {\n      ' \
                            + str(v).replace('\n', '\n      ') + '}\n'
                s += '  }\n'
                s += str(self.total)
                return s
        class StreamStatsDict:
            def __init__(self):
                self.port = dict()
                self.sguid = dict()
            def __repr__(self):
                s = 'port: {\n'
                for k, v in self.port.items():
                    s += str(k) + ': {\n' + str(v) + '}\n'
                s += '}\n'
                s += 'sguid: {\n'
                for k, v in self.sguid.items():
                    s += str(k) + ': {\n' + str(v) + '}\n'
                s += '}\n'
                return s
        ssl = self.getStreamStats(stream_guid_list)
        ssd = StreamStatsDict()
        for ss in ssl.stream_stats:
            if ss.port_id.id not in ssd.port:
                ssd.port[ss.port_id.id] = StreamStatsDictPort()
            assert ss.stream_guid.id not in ssd.port[ss.port_id.id].sguid
            ssd.port[ss.port_id.id].sguid[ss.stream_guid.id] = ss

            ssd.port[ss.port_id.id].total.tx_pkts += ss.tx_pkts
            ssd.port[ss.port_id.id].total.rx_pkts += ss.rx_pkts
            ssd.port[ss.port_id.id].total.tx_bytes += ss.tx_bytes
            ssd.port[ss.port_id.id].total.rx_bytes += ss.rx_bytes

            if ss.stream_guid.id not in ssd.sguid:
                ssd.sguid[ss.stream_guid.id] = StreamStatsDictGuid()
            assert ss.port_id.id not in ssd.sguid[ss.stream_guid.id].port
            ssd.sguid[ss.stream_guid.id].port[ss.port_id.id] = ss
            ssd.sguid[ss.stream_guid.id].total.tx_pkts += ss.tx_pkts
            ssd.sguid[ss.stream_guid.id].total.rx_pkts += ss.rx_pkts
            ssd.sguid[ss.stream_guid.id].total.pkt_loss += \
                    ss.tx_pkts - ss.rx_pkts
        return ssd
