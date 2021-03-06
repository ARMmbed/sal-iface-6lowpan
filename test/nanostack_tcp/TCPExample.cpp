/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed-drivers/mbed.h"
#include "sal/socket_api.h"
#include "sockets/TCPStream.h"

// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "TCPExample"
#include "TCPExample.h"

TCPExample::TCPExample(data_received_cb data_received_callback) :
    sock(SOCKET_STACK_NANOSTACK_IPV6)
{
    _received = false;
    data_recv_callback = data_received_callback;
    sock.open(SOCKET_AF_INET6);
}

TCPExample::~TCPExample()
{
    tr_debug("~TCPExample()");
}

/**
 * Initiate the echoing operation
 * Starts by resolving the address (No DNS supported)
 * @param[in] address The address where to send data
 * @param[in] port port where to data is sent
 * @param[in] testData test data to be sent
 * @param[in] length of data, must be less than BUF_LEN
 *
 * @return SOCKET_ERROR_NONE on success, or an error code on failure
 */
socket_error_t TCPExample::startEcho(const char *address, uint16_t port, const char *testData,  uint16_t length)
{
    _port = port;
    _data_len = length;
    if (length > DATA_BUF_LEN) {
        return SOCKET_ERROR_BAD_ARGUMENT;
    }
    strncpy(_test_data, testData, _data_len);
    /* Start address resolving */
    return sock.resolve(address, TCPStream::DNSHandler_t(this, &TCPExample::onDNS));
}

/**
 * Check if the data has been received
 * @return true if data has been received, false otherwise.
 */
bool TCPExample::isReceived()
{
    return _received;
}

/**
 * Get response data
 * @return Null if no response received, response as a string otherwise.
 */
char *TCPExample::getResponse()
{
    if (isReceived()) {
        return _rxBuf;
    }

    return NULL;
}

/**
 * The DNS Response Handler. Called by the underlying protocol stack
 *
 * @param[in] err status of the address resolving
 */
void TCPExample::onDNS(Socket *s, struct socket_addr sa, const char *domain)
{
    tr_debug("onDNS()");

    (void)s;
    (void)domain;
    /* Check that the result is a valid DNS response */
    /* Start connecting to the remote host */
    _resolvedAddr.setAddr(&sa);
    socket_error_t  err = sock.connect(_resolvedAddr, _port, TCPStream::ConnectHandler_t(this, &TCPExample::onConnect));

    if (err != SOCKET_ERROR_NONE) {
        tr_error("Could not connect!");
    }
}

/**
 * On Connect handler. Called by underlying protocol stack.
 * Once connected, method will send request to the echoing server.
 */
void TCPExample::onConnect(TCPStream *s)
{
    tr_debug("onConnect()");
    /* Send the request */
    (void) s;

    sock.setOnReadable(TCPStream::ReadableHandler_t(this, &TCPExample::onReceive));
    sock.setOnDisconnect(TCPStream::DisconnectHandler_t(this, &TCPExample::onDisconnect));
    socket_error_t err = sock.send(_test_data, _data_len);
    if (err != SOCKET_ERROR_NONE) {
        tr_error("Could not send data!");
    }
}

/**
 * Connection closed handler.
 *
 * @param[in] err status
 */
void TCPExample::onDisconnect(TCPStream *s)
{
    (void)s;
    socket_error_t err = sock.close();
    tr_info("onDisconnect(), err=%d", err);
}

/**
 * Data received handler.
 * Called by the underlying protocol stack when data is available to be read.
 *
 * @param[in] err status of the data receiving
 */
void TCPExample::onReceive(Socket *s)
{
    (void)s;
    size_t nRx = sizeof(_rxBuf);
    tr_debug("onRecv()");
    socket_error_t err = sock.recv(_rxBuf, &nRx);
    tr_info("Received %d bytes", nRx);
    if (err != SOCKET_ERROR_NONE) {
        tr_error("Socket Error %d", err);
    }
    _received = true;
    data_recv_callback();
}
