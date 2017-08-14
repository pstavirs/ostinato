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
"""
This is the core module for the Ostinato Python API.
All drone configuration is done by creating an instance of the
`DroneProxy` class and calling its various methods subsequently.
"""

import os
from rpc import OstinatoRpcChannel, OstinatoRpcController, RpcError
import protocols.protocol_pb2 as ost_pb
import protocols.emulproto_pb2 as emul
from __init__ import __version__

class DroneProxy(object):
    """
    DroneProxy acts as a proxy to a Drone instance. A method invoked on this
    class will be trigerred on the actual Drone instance being proxied
    """

    def __init__(self, host_name, port_number=7878):
        """
        Create a DroneProxy object as a proxy to the Drone instance
        running at the specified host and port
        """
        self.host = host_name
        self.port = port_number
        self.channel = OstinatoRpcChannel()
        self.stub = ost_pb.OstService_Stub(self.channel)
        self.void = ost_pb.Void()

        for method in self.stub.GetDescriptor().methods:
            fn = lambda request=self.void, method_name=method.name: \
                self.callRpcMethod(method_name, request)
            self.__dict__[method.name] = fn
            self.__dict__[method.name].__doc__ = 'This is a protobuf API'

    def hostName(self):
        """
        Returns the hostname of the Drone which is being proxied by
        this DroneProxy object
        """
        return self.host

    def portNumber(self):
        """
        Returns the TCP port number of the Drone which is being proxied by
        this DroneProxy object
        """
        return self.port

    def connect(self):
        """
        Connect to the Drone instance
        """
        self.channel.connect(self.host, self.port)
        ver = ost_pb.VersionInfo()
        ver.client_name = 'python-ostinato'
        ver.version = __version__
        compat = self.checkVersion(ver)
        if compat.result == ost_pb.VersionCompatibility.kIncompatible:
            raise RpcError('incompatible version %s (%s)' % 
                    (ver.version, compat.notes))

    def disconnect(self):
        """
        Disconnect from the Drone instance
        """
        self.channel.disconnect()

    def callRpcMethod(self, method_name, request):
        controller = OstinatoRpcController()
        ost_pb.OstService_Stub.__dict__[method_name](
                    self.stub, controller, request, None)
        return controller.response

    def saveCaptureBuffer(self, buffer, file_name):
        """
        Save the capture buffer in a PCAP file
        """
        f= open(file_name, 'wb')
        f.write(buffer)
        f.flush()
        os.fsync(f.fileno())
        f.close()

