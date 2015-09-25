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
/** \file main.cpp
 *  \brief An example TCP Server application
 *  This listens on TCP Port 7 for incoming connections. The server rejects incoming connections
 *  while it has one connection active. Once an incoming connection is received, the connected socket
 *  echos any incoming buffers back to the remote host. On disconnect, the server shuts down the echoing
 *  socket and resumes accepting connections.
 *
 *  This example is implemented as a logic class (TCPEchoServer) wrapping a TCP server socket.
 *  The logic class handles all events, leaving the main loop to just check for disconnected sockets.
 */

#include "mbed.h"
#include "atmel-rf-driver/driverRFPhy.h"    // rf_device_register
#include "mbed-mesh-api/Mesh6LoWPAN_ND.h"
#include "mbed-mesh-api/MeshInterfaceFactory.h"
#include "mbed-net-sockets/TCPListener.h"
#include "mbed-net-socket-abstract/socket_api.h"
#include "mbed-util/FunctionPointer.h"
#include "Timer.h"
#include "minar/minar.h"
#include "RemoteTrigger.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "main_appl"

using namespace mbed::util;

static uint8_t mesh_network_state = MESH_DISCONNECTED;
static Mesh6LoWPAN_ND *mesh_api = NULL;

void start_echo_server(void);

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 64;
}
using namespace mbed::Sockets::v0;

void remote_client_ready_cb(void) {
    tr_info("remote_client_ready_cb()");
    remote_trigger_stop();
}

/*
 * Callback from mesh network, called when network state changes
 */
void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
    if (mesh_network_state == MESH_CONNECTED) {
        tr_info("Connected to mesh network!");
        start_echo_server();
        remote_trigger_start(remote_client_ready_cb);
    } else if (mesh_network_state == MESH_DISCONNECTED) {
        tr_info("TCP echoing done!, end of program!");
    } else {
        tr_error("bad network state");
    }
}

/**
 * \brief TCPEchoServer implements the logic for listening for TCP connections and
 *        echoing characters back to the sender.
 */
class TCPEchoServer {
public:
    /**
     * The TCPEchoServer Constructor
     * Initializes the server socket
     */
    TCPEchoServer():
        _server(NULL), _stream(NULL),
        _disconnect_pending(false), _connection_ready(false)
        {
            _server = new TCPListener(SOCKET_STACK_NANOSTACK_IPV6);
            _server->setOnError(TCPStream::ErrorHandler_t(this, &TCPEchoServer::onError));
        }
    /**
     * Start the server socket up and start listening
     * @param[in] port the port to listen on
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    void start(const uint16_t port)
    {
        do {
            socket_error_t err = _server->open(SOCKET_AF_INET6);
            if (_server->error_check(err)) break;
            err = _server->bind("0,0,0,0", port);
            if (_server->error_check(err)) break;
            err = _server->start_listening(TCPListener::IncomingHandler_t(this, &TCPEchoServer::onIncoming), 5);
            if (_server->error_check(err)) break;
        } while (0);
    }
protected:
    void onError(Socket *s, socket_error_t err) {
        (void) s;
        tr_error("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
        if (_stream != NULL) {
            _stream->close();
        }

    }
    /**
     * onIncomming handles the allocation of new streams when an incoming connection request is received.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server will reject the incoming connection
     */
    void onIncoming(TCPListener *s, void *impl)
    {
        do {
            tr_info("onIncoming(), accept connection");
            if (impl == NULL) {
                onError(s, SOCKET_ERROR_NULL_PTR);
                break;
            }
            _stream = _server->accept(impl);
            if (_stream == NULL) {
                onError(s, SOCKET_ERROR_BAD_ALLOC);
                break;
            }
            /*
             * Note! BSD deviation, at this point _stream socket is accepted but it is not connected yet!
             * This is because NanoStack calls onIncoming() when first SYN is received from the client.
             * Accepting the connection send SYN-ACK to client and then the client needs to send ACK
             * back to the server. When NanoStack receives the ACK it will trigger call to onConnect().
             */
            _stream->setOnConnect(TCPStream::ConnectHandler_t(this, &TCPEchoServer::onConnect));
            _stream->setOnError(TCPStream::ErrorHandler_t(this, &TCPEchoServer::onError));
            _stream->setOnSent (TCPStream::SentHandler_t(this, &TCPEchoServer::onTX));
            _stream->setOnReadable(TCPStream::ReadableHandler_t(this, &TCPEchoServer::onRX));
            _stream->setOnDisconnect(TCPStream::DisconnectHandler_t(this, &TCPEchoServer::onDisconnect));
        } while (0);
    }

    /**
     * onRX handles incoming buffers and returns them to the sender.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server close the connection.
     */
    void onRX(Socket *s) {
        socket_error_t err;
        tr_info("onRX()");
        size_t size = BUFFER_SIZE-1;
        err = s->recv(buffer, &size);
        if (!s->error_check(err)) {
            buffer[size]=0;
            if (_connection_ready == true) {
                if (size == 0) {
                    tr_info("Closing socket as remote end closed the socket");
                    s->close();
                } else {
                    // Sending is possible only when socket is connected!
                    // This is always true if we received receive data
                    err = s->send(buffer,size);
                    if (err != SOCKET_ERROR_NONE) {
                        onError(s, err);
                    } else {
                        tr_info("echoed: %s", buffer);
                    }
                }
            }
        }
    }

    void onTX(Socket *s, uint16_t sent_bytes) {
        (void) s;
        tr_info("onTX(), sent %d", sent_bytes);
    }
    /**
     * onDisconnect deletes closed streams
     */
    void onDisconnect(TCPStream *s) {
        tr_info("onDisconnect()");
        if(s != NULL) {
            delete s;
            // socket closed, delete also server as we are not expecting any more connections
            delete _server;
            _server = NULL;
        }
    }

    void onConnect(TCPStream *s)
    {
        tr_info("onConnect()");
        /* data can be send only when connection is ready */
        (void) s;
        _connection_ready = true;
    }

protected:
    TCPListener* _server;
    TCPStream*   _stream;
    volatile bool _disconnect_pending;
    bool _connection_ready;
protected:
    char buffer[BUFFER_SIZE];
};

void start_echo_server(void)
{
    static TCPEchoServer server;
    {
        FunctionPointer1<void, uint16_t> fp(&server, &TCPEchoServer::start);
        minar::Scheduler::postCallback(fp.bind(ECHO_SERVER_PORT));
    }
}

/**
 * The main loop of the TCP Echo Server example
 * @return 0; however the main loop should never return.
 */
void app_start(int, char **)
{
    mesh_error_t status;
    // set tracing baud rate
    static Serial pc(USBTX, USBRX);
    pc.baud(115200);

    mesh_api = (Mesh6LoWPAN_ND *)MeshInterfaceFactory::createInterface(MESH_TYPE_6LOWPAN_ND);
    tr_info("6LoWPAN TCP Server");
    status = mesh_api->init(rf_device_register(), mesh_network_callback);
    if (status != MESH_ERROR_NONE) {
        tr_error("Mesh network initialization failed %d!", status);
        return;
    }

    status = mesh_api->connect();
    if (status != MESH_ERROR_NONE) {
        tr_error("Can't connect to mesh network!");
        return;
    }
}
