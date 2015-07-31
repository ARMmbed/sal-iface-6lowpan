/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#ifndef TEST_NANOSTACK_UDP_UDPTEST_H_
#define TEST_NANOSTACK_UDP_UDPTEST_H_

#define BUF_LEN 128

using namespace mbed::Sockets::v0;

typedef void (*data_received_cb)(void);

class UDPTest
{
public:
    UDPTest(data_received_cb data_recv_cb);
    ~UDPTest();
    bool isReceived();
    char *getResponse();
    void startEcho(const char *address, uint16_t udpPort, const char *test_data);

protected:
    void onDNS(Socket *s, struct socket_addr sa, const char* domain);
    void onRecvFrom(Socket *s);
    void onRecv(Socket *s);
    UDPSocket sock;
    SocketAddr _resolvedAddr;
    volatile bool received;
    uint16_t _udpPort;

protected:
    char _rxBuf[BUF_LEN+1];
    char _test_data[BUF_LEN];
    uint8_t _data_len;
    data_received_cb data_recv_callback;
};

#endif /* TEST_NANOSTACK_UDP_UDPTEST_H_ */
