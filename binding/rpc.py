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
import logging
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
        self.log = logging.getLogger(__name__)
        self.log.debug('opening socket')

    def connect(self, host, port):
        self.peer = '%s:%d' % (host, port)
        self.log.debug('connecting to %s', self.peer)
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((host, port))
        except socket.error as e:
            error = 'ERROR: Unable to connect to Drone %s (%s)' % (
                    self.peer, str(e))
            print(error)
            raise

    def disconnect(self):
        self.log.debug('closing socket')
        self.sock.close()

    def CallMethod(self, method, controller, request, response_class, done):
        MSG_HDR_SIZE = 8
        MSG_TYPE_REQUEST = 1
        MSG_TYPE_RESPONSE = 2
        MSG_TYPE_BLOB = 3
        MSG_TYPE_ERROR = 4

        error = ''
        try:
            self.log.info('invoking RPC %s(%s): %s', method.name, 
                    type(request).__name__, response_class.__name__)
            if not request.IsInitialized():
                raise RpcError('missing required fields in request')
            self.log.debug('serializing request arg %s', request)
            req = request.SerializeToString()
            hdr = struct.pack('>HHI', MSG_TYPE_REQUEST, method.index, len(req))
            self.log.debug('req.hdr = %r', hdr)
            self.sock.sendall(hdr + req)

            # receive and parse header
            self.log.debug('receiving response hdr')
            hdr = ''
            while len(hdr) < MSG_HDR_SIZE:
                chunk = self.sock.recv(MSG_HDR_SIZE - len(hdr))
                if chunk == '':
                    raise PeerClosedConnError('connection closed by peer')
                hdr = hdr + chunk

            (msg_type, method_index, resp_len) = struct.unpack('>HHI', hdr)
            self.log.debug('resp hdr: type = %d, method = %d, len = %d',
                    msg_type, method_index, resp_len)

            # receive and parse the actual response message
            self.log.debug('receiving response data')
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

            if msg_type == MSG_TYPE_RESPONSE:
                    response = response_class()
                    response.ParseFromString(resp)
                    self.log.debug('parsed response %s', response)
            elif msg_type == MSG_TYPE_BLOB:
                response = resp
            elif msg_type == MSG_TYPE_ERROR:
                raise RpcError(unicode(resp, 'utf-8'))
            else:
                raise RpcError('unknown RPC msg type %d' % msg_type)

            controller.response = response

        except socket.error as e:
            error = 'ERROR: RPC %s() to Drone %s failed (%s)' % (
                    method.name, self.peer, e)
            self.log.exception(error+e)
            raise
        except PeerClosedConnError as e:
            error = 'ERROR: Drone %s closed connection receiving reply ' \
                  'for RPC %s() (%s)' % (
                    self.peer, method.name, e)
            self.log.exception(error)
            raise
        except EncodeError as e:
            error = 'ERROR: Failed to serialize %s arg for RPC %s() ' \
                  'to Drone %s (%s)' % (
                    type(request).__name__, method.name, self.peer, e)
            self.log.exception(error)
            raise
        except DecodeError as e:
            error = 'ERROR: Failed to parse %s response for RPC %s() ' \
                  'from Drone %s (%s)' % (
                    type(response).__name__, method.name, self.peer, e)
            self.log.exception(error)
            raise
        except RpcMismatchError as e:
            error = 'ERROR: Rpc Mismatch for RPC %s() (%s)' % (
                    method.name, e)
            self.log.exception(error)
            raise
        except RpcError as e:
            error = 'ERROR: error received for RPC %s() (%s) ' % (
                    method.name, e)
            self.log.exception(error)
            raise
        finally:
            if error:
                print(error)



