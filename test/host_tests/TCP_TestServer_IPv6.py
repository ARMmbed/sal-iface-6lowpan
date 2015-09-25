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
cmd_client_socket = "#TRIGGER_TCP_CLIENT:"
server_socket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
client_socket = None

#Reply with source port
def reply_source_port(sock, address):
    print "reply source port:", address[1]
    data = str(address[1])
    sock.send(data)
    print '->Source port:', address[1]

#Echo data until socket is closed
def echo_data_loop(sock, data):
    print "echoing bytes:", len(data)
    sock.sendall(data)
    while True:
        try:
            data = sock.recv(4096)
        except:
            data = None
        if not data:
            break
        print "echoing bytes:", len(data)
        sock.sendall(data)

#TCP client
def start_tcp_client(sock, address,loop_count):
    print "Start TCP client", address[0]
    tcp_client_sock=socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    print "connect..."
    tcp_client_sock.connect((address[0], 7))
    while loop_count:
        print "send data..."
        tcp_client_sock.send("Hello server#" + str(loop_count))
        data=tcp_client_sock.recv(1024)
        print "Received:", data
        loop_count -=1
    tcp_client_sock.close

def runTCPServer():
    host = ''
    port = 50000
    backlog = 5
    size = 1024
    print 'IPv6 TCP Test Server'
    print "Open socket"
    #server_socket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    print "Bind socket"
    server_socket.bind((host,port))
    print "Start listening port:", port
    server_socket.listen(backlog)

    while 1:
        client, address = server_socket.accept()
        client_socket = client
        print 'Connected from:', address[0], 'port:', address[1]
        data = client.recv(size)
        if not data:
            print 'No data received'
        else:
            print 'data received...'
            if data.startswith(cmd_reply_source_port):
                reply_source_port(client, address)
            elif data[0] == "\x00":
                echo_data_loop(client, data)
            elif data.startswith(cmd_client_socket):
                client.sendall(data)
                start_tcp_client(client, address,2)
            else:
                print 'echoing...'
                client.sendall(data)
                time.sleep(1)
        client.close()
        client_socket = None
        print 'client socket closed'

if __name__ == '__main__':
    try:
        runTCPServer()
    except KeyboardInterrupt:
        print 'Close server socket'
        server_socket.close
        if client_socket is not None:
            client_socket.close
            print 'Close client socket'
