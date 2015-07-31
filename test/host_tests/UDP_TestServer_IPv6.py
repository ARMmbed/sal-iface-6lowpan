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
# IPv6 UDP test server

import socket
import time

cmd_reply_echo = "#ECHO:"
cmd_reply5 = "#REPLY5:"
cmd_reply_port = "#REPLY_DIFF_PORT:"

TEST_PORT=50001
REPLY_PORT=60000

def replyFiveTimes(sock, address, data):
    for i in range(0,5):
        sock.sendto(data, address)
        print '->reply#', i
        time.sleep(1)
    print '->multireply:', time.ctime()

def replyGeneratedData(sock, address, arraysize=1280):
    ba = bytearray(arraysize)
    for m in range(arraysize):
        ba[m]=m%256
    sock.sendto(ba, address)
    print '->replyGeneratedData:', time.ctime()

def reply2port(sock, address, data):
    print '->reply2port:', REPLY_PORT
    sock.sendto(data, (address[0], REPLY_PORT))
    print '->reply2port:', time.ctime()

def replyEcho(sock, address, data):
    print '->replyEcho', data
    sock.sendto(data, address)
    print '->replyEcho:', time.ctime()

def runConnectClose():
    print 'IPv6 UDP Test Server'
    Sv4 = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    Sv4.bind(('', TEST_PORT))
    print 'Listening port:', TEST_PORT
    while True:
        (data, address) = Sv4.recvfrom(4096)
        #print 'received from addr: ', address
        if data and address:
            print 'Bytes received:', len(data)
            if data.startswith(cmd_reply5):
                replyFiveTimes(Sv4, address, data)
            elif data.startswith(cmd_reply_port):
                reply2port(Sv4, address, data)
            elif data.startswith(cmd_reply_echo):
                replyEcho(Sv4, address, data)
            else:
                Sv4.sendto(data, address)
                print '->replied:', time.ctime()

if __name__ == '__main__':
    runConnectClose()
