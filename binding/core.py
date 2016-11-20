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
        Convenience wrapper method for getStreamStats which returns StreamStats
        as a dictionary instead of a List
        TODO: document dictionary structure
        """
        class StreamStatsDict:
            def __repr__(self):
                s = 'port: {\n'
                for k, v in self.port.items():
                    s += str(k) + ': {\n' + str(v) + '}\n'
                s += '}\n'
                return s
        class StreamStatsDictPort:
            def __repr__(self):
                s = '  sguid: { \n'
                for k, v in self.sguid.items():
                    s += '    ' + str(k) + ': {\n      ' \
                            + str(v).replace('\n', '\n      ') + '}\n'
                s += '  }\n'
                return s
        ssl = self.getStreamStats(stream_guid_list)
        ssd = StreamStatsDict()
        # TODO: ssd.aggr = dict()
        ssd.port = dict()
        for ss in ssl.stream_stats:
            if ss.port_id.id not in ssd.port:
                ssd.port[ss.port_id.id] = StreamStatsDictPort()
                # TODO: ssd.port[ss.port_id.id].aggr = dict()
                ssd.port[ss.port_id.id].sguid = dict()
            assert ss.stream_guid.id not in ssd.port[ss.port_id.id].sguid
            ssd.port[ss.port_id.id].sguid[ss.stream_guid.id] = ss
        return ssd
