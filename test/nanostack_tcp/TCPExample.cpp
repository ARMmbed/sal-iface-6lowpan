/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#include "mbed.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-sockets/TCPStream.h>
#include "../../mbed-6lowpan-adaptor/mesh_interface.h"

// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "TCPExample"
#include "TCPExample.h"

TCPExample::TCPExample() :
        sock(SOCKET_STACK_NANOSTACK_IPV6)
{
    _received = false;
    sock.open(SOCKET_AF_INET6);
}

TCPExample::~TCPExample()
{
    tr_debug("Destructor()");
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
    if (length > DATA_BUF_LEN)
    {
        return SOCKET_ERROR_BAD_ARGUMENT;
    }
    strncpy(_test_data, testData, _data_len);
    /* Start address resolving */
    return sock.resolve(address, handler_t(this, &TCPExample::onDNS));
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
    if (isReceived())
    {
        return _rxBuf;
    }

    return NULL;
}

/**
 * The DNS Response Handler. Called by the underlying protocol stack
 *
 * @param[in] err status of the address resolving
 */
void TCPExample::onDNS(socket_error_t err)
{
    socket_event_t *e = sock.getEvent();
    tr_debug("onDNS()");
    /* Check that the result is a valid DNS response */
    /* Start connecting to the remote host */
    _resolvedAddr.setAddr(&e->i.d.addr);
    err = sock.connect(&_resolvedAddr, _port,
            handler_t(this, &TCPExample::onConnect));

    if (err != SOCKET_ERROR_NONE)
    {
        tr_error("Could not connect!");
    }

}

/**
 * On Connect handler. Called by underlying protocol stack.
 * Once connected, method will send request to the echoing server.
 */
void TCPExample::onConnect(socket_error_t err) {
    tr_debug("onConnect()");
    /* Send the request */
    sock.setOnReadable(handler_t(this, &TCPExample::onReceive));
    sock.setOnDisconnect(handler_t(this, &TCPExample::onDisconnect));
    err = sock.send(_test_data, _data_len);
    if (err != SOCKET_ERROR_NONE) {
        tr_error("Could not send data!");
    }
}

/**
 * Connection closed handler.
 *
 * @param[in] err status
 */
void TCPExample::onDisconnect(socket_error_t err)
{
    err = sock.close();
    tr_info("onDisconnect(), err=%d", err);
}

/**
 * Data received handler.
 * Called by the underlying protocol stack when data is available to be read.
 *
 * @param[in] err status of the data receiving
 */
void TCPExample::onReceive(socket_error_t err) {
    size_t nRx = sizeof(_rxBuf);
    tr_debug("onRecv()");
    err = sock.recv(_rxBuf, &nRx);
    tr_info("Received %d bytes", nRx);
    if (err != SOCKET_ERROR_NONE) {
        tr_error("Socket Error %d", err);
    }
    _received = true;
}
