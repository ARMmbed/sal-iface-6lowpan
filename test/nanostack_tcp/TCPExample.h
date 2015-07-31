/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#ifndef TEST_NANOSTACK_TCPEXAMPLE_H_
#define TEST_NANOSTACK_TCPEXAMPLE_H_

#define DATA_BUF_LEN 255

using namespace mbed::Sockets::v0;

typedef void (*data_received_cb)(void);

class TCPExample
{
public:
    TCPExample(data_received_cb data_received_callback);
    ~TCPExample();
    bool isReceived();
    char *getResponse();
    socket_error_t startEcho(const char *address, uint16_t port, const char *test_data, uint16_t length);

protected:
    void onDNS(Socket *s, struct socket_addr sa, const char* domain);
    void onConnect(TCPStream *s);
    void onReceive(Socket* s);
    void onDisconnect(TCPStream *s);
    TCPStream sock;
    SocketAddr _resolvedAddr;
    volatile bool _received;
    uint16_t _port;

protected:
    char _rxBuf[DATA_BUF_LEN];
    char _test_data[DATA_BUF_LEN];
    uint16_t _data_len;
    data_received_cb data_recv_callback;
};

#endif /* TEST_NANOSTACK_TCPEXAMPLE_H_ */
