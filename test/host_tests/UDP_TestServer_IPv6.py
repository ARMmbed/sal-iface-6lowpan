#!/usr/bin/env python

# IPv6 UDP test server

import socket
import time

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
            else:
                Sv4.sendto(data, address)
                print '->replied:', time.ctime()

if __name__ == '__main__':
    runConnectClose()
