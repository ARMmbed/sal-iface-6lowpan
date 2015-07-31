/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#include "mbed.h"
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include "UDPTest.h"


// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "UDPTtest"

UDPTest::UDPTest(data_received_cb data_recv_cb) :
        sock(SOCKET_STACK_NANOSTACK_IPV6)
{
    data_recv_callback = data_recv_cb;
    sock.open(SOCKET_AF_INET6);
}

UDPTest::~UDPTest()
{
    tr_debug("Destructor()");
}

/**
 * Initiate the echoing operation
 * Starts by resolving the address (No DNS supported)
 * @param[in] address The address where to send UDP data
 * @param[in] port port where to data is sent
 * @param[in] testData test data to be sent
 *
 * @return SOCKET_ERROR_NONE on success, or an error code on failure
 */
void UDPTest::startEcho(const char *address, uint16_t udpPort, const char *testData)
{
    received = false;
    _udpPort = udpPort;
    _data_len = strlen(testData);
    if (_data_len > BUF_LEN)
    {
        tr_error("too long data");
    }
    strncpy(_test_data, testData, _data_len);
    /* Start the DNS operation */
    sock.resolve(address, Socket::DNSHandler_t(this, &UDPTest::onDNS));
}

/**
 * Check if the UDP data has been received
 * @return true if data has been received, false otherwise.
 */
bool UDPTest::isReceived()
{
    return received;
}

/**
 * Get response data
 * @return Null if no response received, response as a string otherwise.
 */
char *UDPTest::getResponse()
{
    if (isReceived())
    {
        _rxBuf[_data_len] = 0;
        return _rxBuf;
    }

    return NULL;
}

/**
 * The DNS Response Handler. Called by the underlaying protocol stack
 *
 */
void UDPTest::onDNS(Socket *s, struct socket_addr sa, const char* domain)
{
    (void) domain;
    (void) s;
    _resolvedAddr.setAddr(&sa);

    /* Register the read handler */
    sock.setOnReadable(UDPSocket::ReadableHandler_t(this, &UDPTest::onRecvFrom));
    //sock.setOnReadable(handler_t(this, &UDPTest::onRecv));
    /* Send packet to the remote host */
    tr_info("Sending %d bytes of UDP data...", _data_len);
    socket_error_t err = sock.send_to(_test_data, _data_len, &_resolvedAddr, _udpPort);
    if (err != SOCKET_ERROR_NONE)
    {
        tr_error("Socket Error %d", err);
    }
}

/**
 * Data received indication handler
 * Called by the underlaying protocol stack
 */
void UDPTest::onRecvFrom(Socket *s)
{
    (void)s;
    /* Initialize the buffer size */
    size_t nRx = sizeof(_rxBuf);
    tr_debug("onRecvFrom()");

    /* Receive some bytes */
    socket_error_t err = sock.recv_from(_rxBuf, &nRx, &_resolvedAddr, &_udpPort);
    /* A failure on recv is a fatal error in this example */
    if (err != SOCKET_ERROR_NONE)
    {
        tr_error("Socket Error %d", err);
    }
    else
    {
        _rxBuf[nRx] = 0;
        tr_info("Receive port %d", _udpPort);
        tr_info("Receive length %d", nRx);
        tr_info("Received data:\n\r%s", _rxBuf);
    }

    received = true;
    data_recv_callback();
}

/**
 * Data received indication handler
 * Called by the underlying protocol stack
 *
 */
void UDPTest::onRecv(Socket *s) {
    (void)s;
    size_t nRx = sizeof(_rxBuf);
    tr_debug("onRecv()");
    socket_error_t err = sock.recv(_rxBuf, &nRx);
    tr_info("Received %d bytes", nRx);
    if (err != SOCKET_ERROR_NONE) {
        tr_error("Socket Error %d", err);
    }
    else
    {
        _rxBuf[nRx] = 0;
        tr_info("Received data:\n\r%s", _rxBuf);
    }

    received = true;
    data_recv_callback();
}
