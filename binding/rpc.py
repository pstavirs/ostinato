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

from google.protobuf.service import RpcChannel
from google.protobuf.service import RpcController
import socket
import struct

class OstinatoRpcController(RpcController):
    def __init__(self):
        super(OstinatoRpcController, self).__init__()

class OstinatoRpcChannel(RpcChannel):
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self, host, port):
        self.sock.connect((host, port))

    def CallMethod(self, method, controller, request, response_class, done):
        OST_PB_MSG_HDR_SIZE = 8
        OST_PB_MSG_TYPE_REQUEST = 1
        req = request.SerializeToString()
        self.sock.sendall(struct.pack('>HHI', 
            OST_PB_MSG_TYPE_REQUEST, method.index, len(req)) + req)

        hdr = ''
        while len(hdr) < OST_PB_MSG_HDR_SIZE:
            chunk = self.sock.recv(OST_PB_MSG_HDR_SIZE - len(hdr))
            if chunk == '':
                raise RuntimeError("socket connection broken")
            hdr = hdr + chunk

        (type, method, resp_len) = struct.unpack('>HHI', hdr)

        resp = ''
        while len(resp) < resp_len:
            chunk = self.sock.recv(resp_len - len(resp))
            if chunk == '':
                raise RuntimeError("socket connection broken")
            resp = resp + chunk

        response = response_class()
        response.ParseFromString(resp)

        controller.response = response



