/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-sockets/TCPStream.h>
#include "RemoteTrigger.h"

// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "trigger"

// Remote server port and address
#define REMOTE_SERVER_PORT   50000
#define REMOTE_SERVER_ADDR   "FD00:FF1:CE0B:A5E0:21B:38FF:FE90:ABB1"
#define CMD_TRIGGER_CLIENT   "#TRIGGER_TCP_CLIENT:"

static remote_client_cb callback;
class RemoteTrigger;
static RemoteTrigger *remoteTrigger;

using namespace mbed::Sockets::v0;

class RemoteTrigger
{
public:
    RemoteTrigger() : _trigger_socket(SOCKET_STACK_NANOSTACK_IPV6) {}

    void start(void) {
        tr_info("Start triggering socket...");
        socket_error_t err = _trigger_socket.open(SOCKET_AF_INET6);
        if (err != SOCKET_ERROR_NONE) {
            tr_error("Could not create socket!");
        } else {
            _trigger_socket.resolve(REMOTE_SERVER_ADDR, TCPStream::DNSHandler_t(this, &RemoteTrigger::onDNS));
        }
    }

    void stop() {
        tr_info("Stop(close) triggering socket...");
        _trigger_socket.close();
    }

protected:
    void onDNS(Socket *s, struct socket_addr sa, const char *domain)
    {
        (void)s;
        (void)domain;
        _resolvedAddr.setAddr(&sa);
        socket_error_t  err = _trigger_socket.connect(_resolvedAddr, REMOTE_SERVER_PORT, TCPStream::ConnectHandler_t(this, &RemoteTrigger::onConnect));
        if (err != SOCKET_ERROR_NONE) {
            tr_error("Could not connect!");
        }
    }

    void onConnect(TCPStream *s)
    {
        (void) s;
        _trigger_socket.setOnReadable(TCPStream::ReadableHandler_t(this, &RemoteTrigger::onReceive));
        _trigger_socket.setOnDisconnect(TCPStream::DisconnectHandler_t(this, &RemoteTrigger::onDisconnect));
        socket_error_t err = _trigger_socket.send(CMD_TRIGGER_CLIENT, sizeof(CMD_TRIGGER_CLIENT));
        if (err != SOCKET_ERROR_NONE) {
            tr_error("Could not send command!");
        }
    }

    void onReceive(Socket *s)
    {
        (void)s;
        uint8_t buf[100];
        tr_info("trigger-onReceive()");
        size_t len = sizeof(buf);
        socket_error_t err = _trigger_socket.recv((void*)buf, &len);
        if (err != SOCKET_ERROR_NONE) {
            tr_error("Socket Error %d", err);
            return;
        }

        if (len == 0) {
            tr_info("recv len=0, close triggering socket...");
            _trigger_socket.close();
            return;
        }

        // validate that we received the right trigger
        if (memcmp(CMD_TRIGGER_CLIENT, buf, sizeof(CMD_TRIGGER_CLIENT)) == 0) {
            callback();
        }
    }

    void onDisconnect(TCPStream *s)
    {
        (void)s;
        tr_info("trigger socket disconnected, destroy socket");
        delete remoteTrigger;
        remoteTrigger = NULL;
    }

private:
    TCPStream _trigger_socket;
    SocketAddr _resolvedAddr;
};


void remote_trigger_start(remote_client_cb cb) {
    callback = cb;
    remoteTrigger = new RemoteTrigger();
    remoteTrigger->start();
}

void remote_trigger_stop() {
    if (remoteTrigger) {
        remoteTrigger->stop();
    }
}
