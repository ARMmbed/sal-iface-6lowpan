#!/usr/bin/env python
#
# Copyright (c) 2015, ARM Limited, All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# IPv6 TCP test server

import socket
import time

cmd_reply_source_port = "#REPLY_BOUND_PORT:"

def reply_source_port(sock, address):
    data = str(address[1])
    sock.send(data)
    print '->Source port:', address[1]

host = ''
port = 50000
backlog = 5
size = 1024
print 'IPv6 TCP Test Server'
print "Open socket"
s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
print "Bind socket"
s.bind((host,port))
print "Start listening port:", port
s.listen(backlog)
while 1:
    client, address = s.accept()
    print 'Connected from:', address[0], 'port:', address[1]
    time.sleep(1)
    data = client.recv(size)
    if not data:
        print 'No data received'
    else:
        print 'received data:', data
        if data.startswith(cmd_reply_source_port):
                reply_source_port(client, address)
        else:
            client.sendall(data)
            time.sleep(1)
    client.close()
    print 'socket closed'
