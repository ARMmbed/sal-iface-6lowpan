/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#ifndef TEST_NANOSTACK_UDP_UDPTEST_H_
#define TEST_NANOSTACK_UDP_UDPTEST_H_

#define BUF_LEN 128

class UDPTest
{
public:
    UDPTest();
    ~UDPTest();
    bool isReceived();
    char *getResponse();
    socket_error_t startEcho(const char *address, uint16_t udpPort, const char *test_data);

protected:
    void onDNS(socket_error_t err);
    void onRecvFrom(socket_error_t err);
    void onRecv(socket_error_t err);
    mbed::UDPSocket sock;
    mbed::SocketAddr _resolvedAddr;
    volatile bool received;
    uint16_t _udpPort;

protected:
    char _rxBuf[BUF_LEN+1];
    char _test_data[BUF_LEN];
    uint8_t _data_len;
};

#endif /* TEST_NANOSTACK_UDP_UDPTEST_H_ */
