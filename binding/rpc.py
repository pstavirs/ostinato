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

from google.protobuf.message import EncodeError, DecodeError
from google.protobuf.service import RpcChannel
from google.protobuf.service import RpcController
import socket
import struct
import sys

class OstinatoRpcController(RpcController):
    def __init__(self):
        super(OstinatoRpcController, self).__init__()

class OstinatoRpcChannel(RpcChannel):
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self, host, port):
        self.peer = '%s:%d' % (host, port)
        try:
            self.sock.connect((host, port))
        except socket.error, e:
            print 'ERROR: Unable to connect to Drone %s (%s)' % (
                    self.peer, str(e))
            sys.exit(1)

    def disconnect(self):
        self.sock.close()

    def CallMethod(self, method, controller, request, response_class, done):
        OST_PB_MSG_HDR_SIZE = 8
        OST_PB_MSG_TYPE_REQUEST = 1
        OST_PB_MSG_TYPE_RESPONSE = 2
        OST_PB_MSG_TYPE_BLOB = 3

        try:
            req = request.SerializeToString()
            self.sock.sendall(struct.pack('>HHI', 
                OST_PB_MSG_TYPE_REQUEST, method.index, len(req)) + req)
        except EncodeError, e:
            print 'ERROR: Failed to serialize %s arg for RPC %s() ' \
                  'to Drone %s (%s)' % (
                    type(request).__name__, method.name, self.peer, e)
            sys.exit(1)
        except socket.error, e:
            print 'ERROR: Failed to invoke RPC %s() to Drone %s (%s)' % (
                    method.name, self.peer, e)
            sys.exit(1)

        # receive and parse header
        try:
            hdr = ''
            while len(hdr) < OST_PB_MSG_HDR_SIZE:
                chunk = self.sock.recv(OST_PB_MSG_HDR_SIZE - len(hdr))
                if chunk == '':
                    raise RuntimeError("socket connection closed by peer")
                hdr = hdr + chunk
        except socket.error, e:
            print 'ERROR: Failed to receive msg reply for RPC %s() ' \
                  'from Drone %s (%s)' % (
                    method.name, self.peer, e)
            sys.exit(1)
        except RuntimeError, e:
            print 'ERROR: Drone %s closed connection receiving msg reply ' \
                  'for RPC %s() (%s)' % (
                    self.peer, method.name, e)
            sys.exit(1)

        (msg_type, method_index, resp_len) = struct.unpack('>HHI', hdr)

        # verify response method is same as the one requested
        if method_index != method.index:
            print 'ERROR: Received Reply for Method %d; expecting reply for ' \
                    'method %d (%s)' % (
                    method_index, method.index, method.name)
            sys.exit(1)

        # receive and parse the actual response message
        try:
            resp = ''
            while len(resp) < resp_len:
                chunk = self.sock.recv(resp_len - len(resp))
                if chunk == '':
                    raise RuntimeError("socket connection closed by peer")
                resp = resp + chunk
        except socket.error, e:
            print 'ERROR: Failed to receive reply for RPC %s() ' \
                  'from Drone %s (%s)' % (
                    method.name, self.peer, e)
            sys.exit(1)
        except RuntimeError, e:
            print 'ERROR: Drone %s closed connection receiving reply ' \
                  'for RPC %s() (%s)' % (
                    self.peer, method.name, e)
            sys.exit(1)

        if msg_type == OST_PB_MSG_TYPE_RESPONSE:
            try: 
                response = response_class()
                response.ParseFromString(resp)
            except DecodeError, e:
                print 'ERROR: Failed to parse %s response for RPC %s() ' \
                      'from Drone %s (%s)' % (
                        type(response).__name__, method.name, self.peer, e)
                sys.exit(1)
        elif msg_type == OST_PB_MSG_TYPE_BLOB:
            response = resp
        else:
            print 'ERROR: unsupported msg type %d received in respone to %s' % \
                    msg_type, method.name

        controller.response = response



