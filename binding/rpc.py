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

class PeerClosedConnError(Exception):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class RpcError(Exception):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class RpcMismatchError(Exception):
    def __init__(self, msg, received, expected):
        self.msg = msg
        self.received = received
        self.expected = expected
    def __str__(self):
        return '%s - Expected method %d, Received method %d' % (
                self.msg, self.expected, self.received)

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
            error = 'ERROR: Unable to connect to Drone %s (%s)' % (
                    self.peer, str(e))
            print error
            raise

    def disconnect(self):
        self.sock.close()

    def CallMethod(self, method, controller, request, response_class, done):
        OST_PB_MSG_HDR_SIZE = 8
        OST_PB_MSG_TYPE_REQUEST = 1
        OST_PB_MSG_TYPE_RESPONSE = 2
        OST_PB_MSG_TYPE_BLOB = 3

        error = ''
        try:
            req = request.SerializeToString()
            self.sock.sendall(struct.pack('>HHI', 
                OST_PB_MSG_TYPE_REQUEST, method.index, len(req)) + req)

            # receive and parse header
            hdr = ''
            while len(hdr) < OST_PB_MSG_HDR_SIZE:
                chunk = self.sock.recv(OST_PB_MSG_HDR_SIZE - len(hdr))
                if chunk == '':
                    raise PeerClosedConnError('connection closed by peer')
                hdr = hdr + chunk

            (msg_type, method_index, resp_len) = struct.unpack('>HHI', hdr)

            # receive and parse the actual response message
            resp = ''
            while len(resp) < resp_len:
                chunk = self.sock.recv(resp_len - len(resp))
                if chunk == '':
                    raise PeerClosedConnError('connection closed by peer')
                resp = resp + chunk

            # verify response method is same as the one requested
            if method_index != method.index:
                raise RpcMismatchError('RPC mismatch', 
                        expected = method.index, received = method_index)

            if msg_type == OST_PB_MSG_TYPE_RESPONSE:
                    response = response_class()
                    response.ParseFromString(resp)
            elif msg_type == OST_PB_MSG_TYPE_BLOB:
                response = resp
            else:
                raise RpcError('unknown RPC msg type %d' % msg_type)

            controller.response = response

        except socket.error, e:
            error = 'ERROR: RPC %s() to Drone %s failed (%s)' % (
                    method.name, self.peer, e)
            raise
        except PeerClosedConnError, e:
            error = 'ERROR: Drone %s closed connection receiving reply ' \
                  'for RPC %s() (%s)' % (
                    self.peer, method.name, e)
            raise
        except EncodeError, e:
            error = 'ERROR: Failed to serialize %s arg for RPC %s() ' \
                  'to Drone %s (%s)' % (
                    type(request).__name__, method.name, self.peer, e)
            raise
        except DecodeError, e:
            error = 'ERROR: Failed to parse %s response for RPC %s() ' \
                  'from Drone %s (%s)' % (
                    type(response).__name__, method.name, self.peer, e)
            raise
        except RpcMismatchError, e:
            error = 'ERROR: Rpc Mismatch for RPC %s() (%s)' % (
                    method.name, e)
            raise
        except RpcError, e:
            error = 'ERROR: Unknown reply received for RPC %s() (%s) ' % (
                    method.name, e)
            raise
        finally:
            if error:
                print error



