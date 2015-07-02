/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#ifndef TEST_NANOSTACK_TCPEXAMPLE_H_
#define TEST_NANOSTACK_TCPEXAMPLE_H_

#define DATA_BUF_LEN 255

class TCPExample
{
public:
    TCPExample();
    ~TCPExample();
    bool isReceived();
    char *getResponse();
    socket_error_t startEcho(const char *address, uint16_t port, const char *test_data, uint16_t length);

protected:
    void onDNS(socket_error_t err);
    void onConnect(socket_error_t err);
    void onReceive(socket_error_t err);
    void onDisconnect(socket_error_t err);
    mbed::TCPStream sock;
    mbed::SocketAddr _resolvedAddr;
    volatile bool _received;
    uint16_t _port;

protected:
    char _rxBuf[DATA_BUF_LEN];
    char _test_data[DATA_BUF_LEN];
    uint16_t _data_len;
};

#endif /* TEST_NANOSTACK_TCPEXAMPLE_H_ */
