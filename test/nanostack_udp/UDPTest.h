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
    void onDNS(Socket *s, struct socket_addr sa, const char *domain);
    void onRecvFrom(Socket *s);
    void onRecv(Socket *s);
    UDPSocket sock;
    SocketAddr _resolvedAddr;
    volatile bool received;
    uint16_t _udpPort;

protected:
    char _rxBuf[BUF_LEN + 1];
    char _test_data[BUF_LEN];
    uint8_t _data_len;
    data_received_cb data_recv_callback;
};

#endif /* TEST_NANOSTACK_UDP_UDPTEST_H_ */
