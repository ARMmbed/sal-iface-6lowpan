/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * This file is copied from:
 * mbed-net-socket-abstract/source/test/socket_abstract_test.cpp
 *
 * This file should be merged to original one once nanoStack tests are ready.
 */

#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-socket-abstract/test/ctest_env.h>
#include <mbed/Timeout.h>
#include <mbed/Ticker.h>
#include <mbed/mbed.h>
#include "test_cases.h"

#ifndef SOCKET_TEST_TIMEOUT
#define SOCKET_TEST_TIMEOUT 1.0f
#endif

#ifndef SOCKET_TEST_SERVER_TIMEOUT
#define SOCKET_TEST_SERVER_TIMEOUT 10.0f
#endif

#ifndef SOCKET_SENDBUF_BLOCKSIZE
#define SOCKET_SENDBUF_BLOCKSIZE 32
#endif

#ifndef SOCKET_SENDBUF_MAXSIZE
#define SOCKET_SENDBUF_MAXSIZE 4096
#endif

#ifndef SOCKET_SENDBUF_ITERATIONS
#define SOCKET_SENDBUF_ITERATIONS 8
#endif

#define CMD_REPLY5_DATA "#REPLY5:"
#define CMD_REPLY5_DATA_LEN (sizeof(CMD_REPLY5_DATA)-1)
#define CMD_REPLY5_DATA_RESP_COUNT 5

#define CMD_REPLY_DIFF_PORT "#REPLY_DIFF_PORT:"
#define CMD_REPLY_DIFF_PORT_LEN (sizeof(CMD_REPLY_DIFF_PORT)-1)
#define CMD_REPLY_DIFF_LOCAL_PORT 60000

struct IPv4Entry{
    const char * test;
    uint32_t expect;
};

struct IPv6Entry{
    const char * test;
    uint32_t expect[4];
};

const struct IPv4Entry IPv4_TestAddresses[] = {
        {"0.0.0.0", 0},
        {"127.0.0.1", 0x0100007f},
        {"8.8.8.8", 0x08080808},
        {"192.168.0.1",0x0100a8c0},
        {"165.90.165.90",0x5aa55aa5},
        {"90.165.90.165",0xa55aa55a},
        {"1.2.3.4", 0x04030201},
        {"4.3.2.1", 0x01020304}
};

const struct IPv6Entry IPv6_TestAddresses[] = {
        {"0:0:0:0:0:0:0:0", {0x00000000, 0x00000000, 0x00000000, 0x00000000}},
        {"0:0:0:0:0:0:0:1", {0x00000000, 0x00000000, 0x00000000, 0x01000000}},
        {"::1",             {0x00000000, 0x00000000, 0x00000000, 0x01000000}},
        {"2003::7634",      {0x00000320, 0x00000000, 0x00000000, 0x34760000}},
        {"1:2:3:4:5:6:7:8", {0x02000100, 0x04000300, 0x06000500, 0x08000700}},
        {"fd00:ff1:ce0b:a5e1:0:ff:fe00", {0xf10f00fd, 0xe1a50bce, 0xff000000, 0x080000fe}},
};

const unsigned nIPv4Entries = sizeof(IPv4_TestAddresses)/sizeof(struct IPv4Entry);
const unsigned nIPv6Entries = sizeof(IPv6_TestAddresses)/sizeof(struct IPv6Entry);

static void create_test_handler(void)
{
}
int socket_api_test_create_destroy(socket_stack_t stack, socket_address_family_t disable_family)
{
    struct socket s;
    int afi, pfi;
    socket_error_t err;
    const struct socket_api * api = socket_get_api(stack);
    TEST_CLEAR();
    if (!TEST_NEQ(api, NULL)) {
        // Test cannot continue without API.
        return 0;
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        return 0;
    }

    // Create a socket for each address family
    for (afi = SOCKET_AF_UNINIT; afi <= SOCKET_AF_MAX; afi++) {
        socket_address_family_t af = static_cast<socket_address_family_t>(afi);
        // Create a socket for each protocol family
        for (pfi = SOCKET_PROTO_UNINIT; pfi <= SOCKET_PROTO_MAX; pfi++) {
            socket_proto_family_t pf = static_cast<socket_proto_family_t>(pfi);
            // Zero the implementation
            s.impl = NULL;
            err = api->create(&s, af, pf, &create_test_handler);
            // catch expected failing cases
            if (af == SOCKET_AF_UNINIT || af == SOCKET_AF_MAX ||
                    pf == SOCKET_PROTO_UNINIT || pf == SOCKET_PROTO_MAX ||
                    af == disable_family) {
                TEST_NEQ(err, SOCKET_ERROR_NONE);
                continue;
            }
            TEST_EQ(err, SOCKET_ERROR_NONE);
            if (!TEST_NEQ(s.impl, NULL)) {
                continue;
            }
            // Destroy the socket;
            err = api->destroy(&s);
            TEST_EQ(err, SOCKET_ERROR_NONE);
            // Zero the implementation
            s.impl = NULL;
            // Create a socket with a NULL handler
            err = api->create(&s, af, pf, NULL);
            TEST_NEQ(err, SOCKET_ERROR_NONE);
            TEST_EQ(s.impl, NULL);
            // A NULL impl is not explicitly an exception since it can be zeroed during disconnect
            // Destroy the socket
            err = api->destroy(&s);
            TEST_EQ(err, SOCKET_ERROR_NONE);
            // Try destroying a socket that hasn't been created
            s.impl = NULL;
            err = api->destroy(&s);
            TEST_EQ(err, SOCKET_ERROR_NONE);

            /*
             * Because the allocator is stack-dependent, there is no way to test for a
             * memory leak in a portable way
             */
        }
    }
    TEST_RETURN();
}

int socket_api_test_socket_str2addr(socket_stack_t stack, socket_address_family_t disable_family)
{
    struct socket s;
    socket_error_t err;
    int afi;
    struct socket_addr addr;
    const struct socket_api * api = socket_get_api(stack);
    s.api = api;
    s.stack = stack;
    // Create a socket for each address family
    for (afi = SOCKET_AF_UNINIT+1; afi < SOCKET_AF_MAX; afi++) {
        socket_address_family_t af = static_cast<socket_address_family_t>(afi);
        if (af == disable_family) {
            continue;
        }
        switch(af) {
            case SOCKET_AF_INET4:
                for (unsigned i = 0; i < nIPv4Entries; i++) {
                    err = api->str2addr(&s, &addr, IPv4_TestAddresses[i].test);
                    TEST_EQ(err, SOCKET_ERROR_NONE);
                    if (!TEST_EQ(addr.storage[0], IPv4_TestAddresses[i].expect)) {
                        printf ("Expected: %08lx, got: %08lx\r\n", IPv4_TestAddresses[i].expect,addr.storage[0]);
                    }
                }
                break;
            case SOCKET_AF_INET6:
                for (unsigned i = 0; i < nIPv6Entries; i++) {
                    err = api->str2addr(&s, &addr, IPv6_TestAddresses[i].test);
                    TEST_EQ(err, SOCKET_ERROR_NONE);
                    if (!(TEST_EQ(addr.storage[0], IPv6_TestAddresses[i].expect[0]) &&
                            TEST_EQ(addr.storage[1], IPv6_TestAddresses[i].expect[1]) &&
                            TEST_EQ(addr.storage[2], IPv6_TestAddresses[i].expect[2]) &&
                            TEST_EQ(addr.storage[3], IPv6_TestAddresses[i].expect[3]))) {
                        printf ("Expected: %08lx.%08lx.%08lx.%08lx, got: %08lx.%08lx.%08lx.%08lx\r\n",
                                IPv6_TestAddresses[i].expect[0], IPv6_TestAddresses[i].expect[1], IPv6_TestAddresses[i].expect[2], IPv6_TestAddresses[i].expect[3],
                                addr.storage[0], addr.storage[1], addr.storage[2], addr.storage[3]);
                    }
                }
                break;
            default:
                TEST_EQ(1,0);
                break;
        }
    }
    TEST_RETURN();
}

volatile int timedout;
static void onTimeout() {
    TEST_PRINT("onTimeout()");
    timedout = 1;
}

static struct socket *ConnectCloseSock;

volatile int eventid;
volatile int closed;
volatile int connected;
static void connect_close_handler(void)
{
    switch(ConnectCloseSock->event->event) {
        case SOCKET_EVENT_DISCONNECT:
            closed = 1;
            break;
        case SOCKET_EVENT_CONNECT:
            connected = 1;
            break;
        default:
            break;
    }
}

int socket_api_test_connect_close(socket_stack_t stack, socket_address_family_t af, const char* server, uint16_t port)
{
    struct socket s;
    int pfi;
    socket_error_t err;
    const struct socket_api * api = socket_get_api(stack);
    struct socket_addr addr;

    ConnectCloseSock = &s;
    TEST_CLEAR();
    if (!TEST_NEQ(api, NULL)) {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    // Create a socket for each protocol family
    for (pfi = SOCKET_PROTO_UNINIT+1; pfi < SOCKET_PROTO_MAX; pfi++) {
        socket_proto_family_t pf = static_cast<socket_proto_family_t>(pfi);
        // Zero the implementation
        s.impl = NULL;
        err = api->create(&s, af, pf, &connect_close_handler);
        // catch expected failing cases
        TEST_EQ(err, SOCKET_ERROR_NONE);
        if (!TEST_NEQ(s.impl, NULL)) {
            continue;
        }
        // Tell the host launch a server
        TEST_PRINT(">>> ES,%d\r\n", pf);

        // connect to a remote host
        err = api->str2addr(&s, &addr, server);
        TEST_EQ(err, SOCKET_ERROR_NONE);

        timedout = 0;
        connected = 0;
        mbed::Timeout to;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        err = api->connect(&s, &addr, port);
        TEST_EQ(err, SOCKET_ERROR_NONE);
        if (err!=SOCKET_ERROR_NONE) {
            printf("err = %d\r\n", err);
        }
        switch (pf) {
        case SOCKET_DGRAM:
            while ((!api->is_connected(&s)) && (!timedout)) {
                __WFI();
            }
            break;
        case SOCKET_STREAM:
            while (!connected && !timedout) {
                __WFI();
            }
            break;
        default: break;
        }
        to.detach();
        TEST_EQ(timedout, 0);

        // close the connection
        timedout = 0;
        closed = 0;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        err = api->close(&s);
        TEST_EQ(err, SOCKET_ERROR_NONE);
        if (err!=SOCKET_ERROR_NONE) {
            printf("err = %d\r\n", err);
        }
        switch (pf) {
            case SOCKET_DGRAM:
                while ((api->is_connected(&s)) && (!timedout)) {
                    __WFI();
                }
                break;
            case SOCKET_STREAM:
                while (!closed && !timedout) {
                    __WFI();
                }
                break;
            default: break;
        }
        to.detach();
        TEST_EQ(timedout, 0);
        // Tell the host to kill the server
        TEST_PRINT(">>> KILL ES\r\n");

        // Destroy the socket
        err = api->destroy(&s);
        TEST_EQ(err, SOCKET_ERROR_NONE);
    }
    TEST_RETURN();
}


static volatile bool blocking_resolve_done;
static volatile socket_error_t blocking_resolve_err;
static volatile struct socket *blocking_resolve_socket;
static volatile struct socket_addr blocking_resolve_addr;
static volatile const char * blocking_resolve_domain;

static void blocking_resolve_cb()
{
    struct socket_event *e = blocking_resolve_socket->event;
    if (e->event == SOCKET_EVENT_ERROR) {
        blocking_resolve_err = e->i.e;
        blocking_resolve_done = true;
        return;
    } else if (e->event == SOCKET_EVENT_DNS) {
        blocking_resolve_addr.type = e->i.d.addr.type;
        blocking_resolve_addr.storage[0] = e->i.d.addr.storage[0];
        blocking_resolve_addr.storage[1] = e->i.d.addr.storage[1];
        blocking_resolve_addr.storage[2] = e->i.d.addr.storage[2];
        blocking_resolve_addr.storage[3] = e->i.d.addr.storage[3];
        blocking_resolve_domain = e->i.d.domain;
        blocking_resolve_err = SOCKET_ERROR_NONE;
        blocking_resolve_done = true;
    } else {
        blocking_resolve_err = SOCKET_ERROR_UNKNOWN;
        blocking_resolve_done = true;
        return;
    }
}

socket_error_t blocking_resolve(const socket_stack_t stack, const socket_address_family_t af, const char* server, struct socket_addr * addr) {
    struct socket s;
    const struct socket_api *api = socket_get_api(stack);
    (void) af;
    blocking_resolve_socket = &s;
    s.stack = stack;
    s.handler = blocking_resolve_cb;
    s.api = api;
    blocking_resolve_done = false;
    socket_error_t err = api->resolve(&s, server);
    if(!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        return err;
    }
    while (!blocking_resolve_done) {
        __WFI();
    }
    if(!TEST_EQ(blocking_resolve_err, SOCKET_ERROR_NONE)) {
        return blocking_resolve_err;
    }
    int rc = strcmp(server, (char *)blocking_resolve_domain);
    TEST_EQ(rc,0);
    memcpy(addr, (const void*)&blocking_resolve_addr, sizeof(struct socket_addr));
    return SOCKET_ERROR_NONE;
}

/**
 * Tests the following APIs:
 * create
 * connect
 * send
 * recv
 * close
 * destroy
 *
 * @param stack
 * @param disable_family
 * @param server
 * @param port
 * @return
 */
static struct socket *client_socket;
static volatile bool client_event_done;
static volatile bool client_rx_done;
static volatile int client_rx_resp_count;
static volatile bool client_tx_done;
static volatile struct socket_tx_info client_tx_info;
static volatile struct socket_event client_event;
static void client_cb() {
    struct socket_event *e = client_socket->event;
    event_flag_t event = e->event;
    switch (event) {
        case SOCKET_EVENT_RX_DONE:
            client_rx_done = true;
            client_rx_resp_count++;
            //TEST_PRINT("SOCKET_EVENT_RX_DONE %d\n\r", client_rx_resp_count);
            break;
        case SOCKET_EVENT_TX_DONE:
            client_tx_done = true;
            client_tx_info.sentbytes = e->i.t.sentbytes;
            break;
        default:
            memcpy((void *) &client_event, (const void*)e, sizeof(e));
            client_event_done = true;
            break;
    }
}

int socket_api_test_echo_client_connected(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf, bool connect,
        const char* server, uint16_t port, run_func_t run_cb, uint16_t max_packet_size)
{
    struct socket s;
    socket_error_t err;
    const struct socket_api *api = socket_get_api(stack);
    client_socket = &s;
    mbed::Timeout to;
    // Create the socket
    TEST_CLEAR();
    TEST_PRINT("\r\n%s af: %d, pf: %d, connect: %d, server: %s:%d\r\n",__func__, (int) af, (int) pf, (int) connect, server, (int) port);

    if (!TEST_NEQ(api, NULL)) {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    struct socket_addr addr;
    // Resolve the host address
    err = blocking_resolve(stack, af, server, &addr);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }
    // Tell the host launch a server
    TEST_PRINT(">>> ES,%d\r\n", pf);
    // Allocate a data buffer for tx/rx
    void *data = malloc(SOCKET_SENDBUF_MAXSIZE);

    // Zero the socket implementation
    s.impl = NULL;
    // Create a socket
    err = api->create(&s, af, pf, &client_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_EXIT();
    }
    // Connect to a host
    if (connect) {
        client_event_done = false;
        timedout = 0;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        err = api->connect(&s, &addr, port);
        if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
            TEST_EXIT();
        }
        // Override event for dgrams.
        if (pf == SOCKET_DGRAM) {
            client_event.event = SOCKET_EVENT_CONNECT;
            client_event_done = true;
        }
        // Wait for onConnect
        while (!timedout && !client_event_done) {
            run_cb();
        }

        // Make sure that the correct event occurred
        if (!TEST_EQ(client_event.event, SOCKET_EVENT_CONNECT)) {
            TEST_EXIT();
        }
        to.detach();
    }

    // For several iterations
    for (size_t i = 0; i < SOCKET_SENDBUF_ITERATIONS; i++) {
        // Fill some data into a buffer
        const size_t nWords = SOCKET_SENDBUF_BLOCKSIZE * (1 << i) / sizeof(uint16_t);
        if (nWords*sizeof(uint16_t) > max_packet_size)
        {
            TEST_PRINT("Packet size %d bigger than max packet size %d\r\n", nWords*sizeof(uint16_t), max_packet_size);
            break;
        }
        for (size_t j = 0; j < nWords; j++) {
            *((uint16_t*) data + j) = j;
        }
        // Send the data
        client_tx_done = false;
        client_rx_done = false;
        timedout = 0;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);

        if(connect) {
            err = api->send(&s, data, nWords * sizeof(uint16_t));
        } else {
            err = api->send_to(&s, data, nWords * sizeof(uint16_t), &addr, port);
        }
        if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
            TEST_PRINT("Failed to send %u bytes\r\n", nWords * sizeof(uint16_t));
        } else {
            size_t tx_bytes = 0;
            do {
                // Wait for the onSent callback
                while (!timedout && !client_tx_done) {
                    run_cb();
                }
                if (!TEST_EQ(timedout,0)) {
                    break;
                }
                if (!TEST_NEQ(client_tx_info.sentbytes, 0)) {
                    break;
                }
                tx_bytes += client_tx_info.sentbytes;
                if (tx_bytes < nWords * sizeof(uint16_t)) {
                    client_tx_done = false;
                    continue;
                }
                to.detach();
                TEST_EQ(tx_bytes, nWords * sizeof(uint16_t));
                break;
            } while (1);
        }
        timedout = 0;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        memset(data, 0, nWords * sizeof(uint16_t));
        // Wait for the onReadable callback
        size_t rx_bytes = 0;
        do {
            while (!timedout && !client_rx_done) {
                run_cb();
            }
            if (!TEST_EQ(timedout,0)) {
                break;
            }
            size_t len = SOCKET_SENDBUF_MAXSIZE - rx_bytes;
            // Receive data
            if (connect) {
                err = api->recv(&s, (void*) ((uintptr_t) data + rx_bytes), &len);
            } else {
                struct socket_addr rxaddr;
                uint16_t rxport = 0;
                // addr may contain unused data in the storage element.
                memcpy(&rxaddr, &addr, sizeof(rxaddr));
                // Receive from...
                err = api->recv_from(&s, (void*) ((uintptr_t) data + rx_bytes), &len, &rxaddr, &rxport);
                TEST_EQ(rxaddr.type, stack);
                // ON IPv6, replies are coming from temporary IP address, only 2 first part are valid
                int rc = memcmp(&rxaddr.storage, &addr.storage, sizeof(rxaddr.storage)/2);
                if(!TEST_EQ(rc, 0)) {
                    TEST_PRINT("Spurious receive packet\r\n");
                }
                TEST_EQ(rxport, port);
            }
            if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
                break;
            }
            rx_bytes += len;
            if (rx_bytes < nWords * sizeof(uint16_t)) {
                client_rx_done = false;
                continue;
            }
            to.detach();
            break;
        } while (1);
        if(!TEST_EQ(rx_bytes, nWords * sizeof(uint16_t))) {
            TEST_PRINT("Expected %u, got %u\r\n", nWords * sizeof(uint16_t), rx_bytes);
        }

        // Validate that the two buffers are the same
        bool match = true;
        size_t j;
        for (j = 0; match && j < nWords; j++) {
            match = (*((uint16_t*) data + j) == j);
        }
        if(!TEST_EQ(match, true)) {
            TEST_PRINT("Mismatch in %u byte packet at offset %u\r\n", nWords * sizeof(uint16_t), j * sizeof(uint16_t));
        }

    }
    if (connect) {
        // close the socket
        client_event_done = false;
        timedout = 0;
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        err = api->close(&s);
        TEST_EQ(err, SOCKET_ERROR_NONE);

        // Override event for dgrams.
        if (pf == SOCKET_DGRAM) {
            client_event.event = SOCKET_EVENT_DISCONNECT;
            client_event_done = true;
        }

        // wait for onClose
        while (!timedout && !client_event_done) {
            run_cb();
        }
        if (TEST_EQ(timedout,0)) {
            to.detach();
        }
        // Make sure that the correct event occurred
        TEST_EQ(client_event.event, SOCKET_EVENT_DISCONNECT);
    }

    // destroy the socket
    err = api->destroy(&s);
    TEST_EQ(err, SOCKET_ERROR_NONE);

test_exit:
    TEST_PRINT(">>> KILL,ES\r\n");
    free(data);
    TEST_RETURN();
}


static volatile bool incoming;
static volatile bool server_event_done;
static volatile bool server_rx_done;
static volatile struct socket *server_socket;
static volatile struct socket_event server_event;
static void server_cb(void)
{
    struct socket_event *e = server_socket->event;
    event_flag_t event = e->event;
    switch (event) {
        case SOCKET_EVENT_ACCEPT:
            incoming = true;
            server_event.event = event;
            server_event.i.a.newimpl = e->i.a.newimpl;
            e->i.a.reject = 0;
            break;
        case SOCKET_EVENT_RX_DONE:
            server_rx_done = true;
            TEST_PRINT("server_cb -  SOCKET_EVENT_RX_DONE\n\r");
            break;
        default:
            server_event_done = true;
            break;
    }
}

int socket_api_test_echo_server_stream(socket_stack_t stack, socket_address_family_t af, const char* local_addr, uint16_t port)
{
    struct socket s;
    struct socket cs;
    struct socket_addr addr;
    socket_error_t err;
    const struct socket_api *api = socket_get_api(stack);
    server_socket = &s;
    client_socket = &cs;
    mbed::Timeout to;
    mbed::Ticker ticker;
    // Create the socket
    TEST_CLEAR();

    if (!TEST_NEQ(api, NULL)) {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    // Zero the socket implementation
    s.impl = NULL;
    // Create a socket
    err = api->create(&s, af, SOCKET_STREAM, &server_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    err = api->str2addr(&s, &addr, local_addr);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    // start the TCP timer
    uint32_t interval_ms = api->periodic_interval(&s);
    TEST_NEQ(interval_ms, 0);
    uint32_t interval_us = interval_ms * 1000;
    socket_api_handler_t periodic_task = api->periodic_task(&s);
    if (TEST_NEQ(periodic_task, NULL)) {
        ticker.attach_us(periodic_task, interval_us);
    }

    err = api->bind(&s, &addr, port);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }
    void *data = malloc(SOCKET_SENDBUF_MAXSIZE);
    if(!TEST_NEQ(data, NULL)) {
        TEST_RETURN();
    }
    // Tell the host test to try and connect
    TEST_PRINT(">>> EC,%s,%d\r\n", local_addr, port);

    bool quit = false;
    // For several iterations
    while (!quit) {
        timedout = false;
        server_event_done = false;
        incoming = false;
        to.attach(onTimeout, SOCKET_TEST_SERVER_TIMEOUT);
        // Listen for incoming connections
        err = api->start_listen(&s, 0);
        if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
            TEST_EXIT();
        }
        // Reset the state of client_rx_done
        client_rx_done = false;
        // Wait for a connect event
        while (!timedout && !incoming) {
            __WFI();
        }
        if (TEST_EQ(timedout,0)) {
            to.detach();
        } else {
            TEST_EXIT();
        }
        if(!TEST_EQ(server_event.event, SOCKET_EVENT_ACCEPT)) {
            TEST_PRINT("Event: %d\r\n",(int)client_event.event);
            continue;
        }
        // Stop listening
        server_event_done = false;
        // err = api->stop_listen(&s);
        // TEST_EQ(err, SOCKET_ERROR_NONE);
        // Accept an incoming connection
        cs.impl = server_event.i.a.newimpl;
        cs.family = s.family;
        cs.stack  = s.stack;
        err = api->accept(&cs, client_cb);
        if(!TEST_EQ(err, SOCKET_ERROR_NONE)) {
            continue;
        }
        to.attach(onTimeout, SOCKET_TEST_SERVER_TIMEOUT);

                    // Client should test for successive connections being rejected
        // Until Client disconnects
        while (client_event.event != SOCKET_EVENT_ERROR && client_event.event != SOCKET_EVENT_DISCONNECT) {
            // Wait for a read event
            while (!client_event_done && !client_rx_done && !timedout) {
                __WFI();
            }
            if (!TEST_EQ(client_event_done, false)) {
                client_event_done = false;
                continue;
            }
            // Reset the state of client_rx_done
            client_rx_done = false;

            // Receive some data
            size_t len = SOCKET_SENDBUF_MAXSIZE;
            err = api->recv(&cs, data, &len);
            if (!TEST_NEQ(err, SOCKET_ERROR_WOULD_BLOCK)) {
                TEST_PRINT("Rx Would Block\r\n");
                continue;
            }
            if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
                TEST_PRINT("err: (%d) %s\r\n", err, socket_strerror(err));
                break;
            }

            // Check if the server should halt
            if (strncmp((const char *)data, "quit", 4) == 0) {
                quit = true;
                break;
            }
            // Send some data
            err = api->send(&cs, data, len);
            if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
                break;
            }
        }
        to.detach();
        TEST_NEQ(timedout, true);

        // Close client socket
        err = api->close(&cs);
        TEST_EQ(err, SOCKET_ERROR_NONE);
    }
    err = api->stop_listen(&s);
    TEST_EQ(err, SOCKET_ERROR_NONE);
test_exit:
    ticker.detach();
    TEST_PRINT(">>> KILL,EC\r\n");
    free(data);
    // Destroy server socket
    TEST_RETURN();
}

/*
 * NanoStack UDP testing
 * */
int ns_udp_test_buffered_recv_from(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
        const char* server, uint16_t port, run_func_t run_cb)
{
    struct socket sock;
    socket_error_t err;
    const struct socket_api *api = socket_get_api(stack);
    client_socket = &sock;
    mbed::Timeout to;
    size_t rx_bytes = 0;
    uint16_t data_len = CMD_REPLY5_DATA_LEN;

    // Create the socket
    TEST_CLEAR();
    TEST_PRINT("\r\n%s af: %d, pf: %d, server: %s:%d\r\n",__func__, (int) af, (int) pf, server, (int) port);

    if (!TEST_NEQ(api, NULL)) {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }

    struct socket_addr addr;
    // Resolve the host address
    err = blocking_resolve(stack, af, server, &addr);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_RETURN();
    }
    // Tell the host launch a server
    TEST_PRINT(">>> ES,%d\r\n", pf);
    // Allocate a data buffer for tx/rx
    char *data = (char*)malloc(SOCKET_SENDBUF_MAXSIZE);

    // Zero the socket implementation
    sock.impl = NULL;
    // Create a socket
    err = api->create(&sock, af, pf, &client_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE)) {
        TEST_EXIT();
    }

    strcpy(data, CMD_REPLY5_DATA);

    // Send the data
    client_tx_done = false;
    client_rx_done = false;
    timedout = 0;
    to.attach(onTimeout, SOCKET_TEST_TIMEOUT);

    err = api->send_to(&sock, data, data_len, &addr, port);

    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_PRINT("Failed to send %u bytes\r\n", data_len);
    }
    else
    {
        size_t tx_bytes = 0;
        do
        {
            // Wait for the onSent callback
            while (!timedout && !client_tx_done)
            {
                run_cb();
            }
            if (!TEST_EQ(timedout, 0))
            {
                break;
            }
            if (!TEST_NEQ(client_tx_info.sentbytes, 0))
            {
                break;
            }
            tx_bytes += client_tx_info.sentbytes;
            to.detach();
            TEST_EQ(tx_bytes, data_len);
            break;
        } while (1);
    }
    timedout = 0;
    to.detach();
    /* adjust timeout as we wait for X responses */
    to.attach(onTimeout, CMD_REPLY5_DATA_RESP_COUNT * SOCKET_TEST_TIMEOUT);
    memset(data, 0, SOCKET_SENDBUF_MAXSIZE);

    // Wait for the onReadable callback
    client_rx_resp_count = 0;
    do
    {
        while (!timedout && client_rx_resp_count < CMD_REPLY5_DATA_RESP_COUNT)
        {
            run_cb();
        }
        if (!TEST_EQ(timedout, 0))
        {
            break;
        }

        timedout = 0;
        to.detach();
        to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
        int iterations = 0;
        data_len = CMD_REPLY5_DATA_LEN;
        size_t len = data_len;
        // Receive data and port
        struct socket_addr rxaddr;
        uint16_t rxport = 0;

        do
        {
            iterations++;
            // addr may contain unused data in the storage element.
            memcpy(&rxaddr, &addr, sizeof(rxaddr));
            // Receive from...
            err = api->recv_from(&sock, (void*) ((uintptr_t) data + rx_bytes),
                    &len, &rxaddr, &rxport);
            TEST_EQ(rxaddr.type, stack);
            // For IPv6 addresses, replies are coming from temporary IPv6 address, only 8 first bytes are the same
            int rc = memcmp(&rxaddr.storage, &addr.storage,
                    sizeof(rxaddr.storage) / 2);
            if (!TEST_EQ(rc, 0))
            {
                TEST_PRINT("Spurious receive packet\r\n");
            }
            TEST_EQ(rxport, port);

            if (!TEST_EQ(err, SOCKET_ERROR_NONE))
            {
                break;
            }

            /* Check received data */
            //TEST_PRINT("%d. recv: %s, expect: %.*s\r\n", iterations, &data[rx_bytes], data_len, TEST_REPLY5_DATA);
            int match = strncmp(&data[rx_bytes], CMD_REPLY5_DATA, data_len);
            if (!TEST_EQ(match, 0))
            {
                TEST_PRINT("Received data does not match! %s", &data[rx_bytes]);
            }
            rx_bytes += len;
            data_len--;
            len = data_len;
        } while (0 == timedout && iterations < CMD_REPLY5_DATA_RESP_COUNT);

        if (!TEST_EQ(timedout, 0))
        {
            break;
        }

        // Receive again, should return SOCKET_ERROR_WOULD_BLOCK now
        err = api->recv_from(&sock, (void*) ((uintptr_t) data + rx_bytes), &len,
                &rxaddr, &rxport);
        if (!TEST_EQ(err, SOCKET_ERROR_WOULD_BLOCK))
        {
            TEST_PRINT("Empty socket reading failed with status %d\r\n", err);
        }

        to.detach();
        break;
    } while (1);

    // destroy the socket
    err = api->destroy(&sock);
    TEST_EQ(err, SOCKET_ERROR_NONE);

test_exit:
    TEST_PRINT(">>> KILL,ES\r\n");
    free(data);
    TEST_RETURN();
}

int ns_socket_test_bind(socket_stack_t stack, socket_address_family_t af,
        socket_proto_family_t pf, const char* server, uint16_t port,
        run_func_t run_cb)
{
    struct socket sock_client;
    struct socket sock_srv;
    socket_error_t err;
    const struct socket_api *api = socket_get_api(stack);
    client_socket = &sock_client;
    server_socket = &sock_srv;
    mbed::Timeout to;
    uint16_t data_len = CMD_REPLY_DIFF_PORT_LEN;

    // Create the socket
    TEST_CLEAR();
    TEST_PRINT("\r\n%s af: %d, pf: %d, server: %s:%d\r\n",  __func__, (int ) af, (int ) pf, server, (int ) port);

    if (!TEST_NEQ(api, NULL))
    {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_RETURN();
    }

    struct socket_addr addr;
    // Resolve the host address
    err = blocking_resolve(stack, af, server, &addr);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_RETURN();
    }
    // Tell the host launch a server
    TEST_PRINT(">>> ES,%d\r\n", pf);
    // Allocate a data buffer for tx/rx
    void *data = malloc(SOCKET_SENDBUF_MAXSIZE);
    strcpy((char*)data, CMD_REPLY_DIFF_PORT);


    // Zero the socket implementation
    sock_client.impl = NULL;
    sock_srv.impl = NULL;
    // Create client socket
    err = api->create(&sock_client, af, pf, &client_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_EXIT();
    }
    // Create server socket
    err = api->create(&sock_srv, af, pf, &server_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_EXIT();
    }

    // bind server socket to port
    socket_addr address;
    memset(&address.storage[0], 0, sizeof(address.storage));
    address.type = SOCKET_STACK_NANOSTACK_IPV6;
    err = api->bind(&sock_srv, &address, CMD_REPLY_DIFF_LOCAL_PORT);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_EXIT();
    }

    // Send the data
    client_tx_done = false;
    server_rx_done = false;
    timedout = 0;
    to.attach(onTimeout, SOCKET_TEST_TIMEOUT);

    err = api->send_to(&sock_client, data, data_len, &addr, port);

    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_PRINT("Failed to send %u bytes\r\n", data_len);
    }
    else
    {
        size_t tx_bytes = 0;
        do
        {
            // Wait for the onSent callback
            while (!timedout && !client_tx_done)
            {
                run_cb();
            }
            if (!TEST_EQ(timedout, 0))
            {
                break;
            }
            if (!TEST_NEQ(client_tx_info.sentbytes, 0))
            {
                break;
            }
            tx_bytes += client_tx_info.sentbytes;
            to.detach();
            TEST_EQ(tx_bytes, data_len);
            break;
        } while (1);
    }
    timedout = 0;
    to.attach(onTimeout, SOCKET_TEST_TIMEOUT);
    memset(data, 0, SOCKET_SENDBUF_MAXSIZE);
    // Wait for the onReadable callback
    do
    {
        while (!timedout && !server_rx_done)
        {
            run_cb();
        }

        if (!TEST_EQ(timedout, 0))
        {
            break;
        }

        size_t len = SOCKET_SENDBUF_MAXSIZE;
        // Receive data
        struct socket_addr rxaddr;
        uint16_t rxport = 0;
        memcpy(&rxaddr, &addr, sizeof(rxaddr));
        // Receive from...
        err = api->recv_from(&sock_srv, data, &len, &rxaddr, &rxport);
        TEST_EQ(rxaddr.type, stack);
        // ON IPv6, replies are coming from temporary IP address, only 2 first part are valid
        int rc = memcmp(&rxaddr.storage, &addr.storage, sizeof(rxaddr.storage) / 2);
        if (!TEST_EQ(rc, 0))
        {
            TEST_PRINT("Spurious receive packet\r\n");
        }

        TEST_EQ(rxport, port);

        if (!TEST_EQ(err, SOCKET_ERROR_NONE))
        {
            break;
        }

        if (!TEST_EQ(len, CMD_REPLY_DIFF_PORT_LEN))
        {
            TEST_PRINT("Expected %u, got %u\r\n", CMD_REPLY_DIFF_PORT_LEN, len);
        }

        int match = strcmp((char*)data, CMD_REPLY_DIFF_PORT);
        if (!TEST_EQ(match, 0))
        {
            TEST_PRINT("Expected %s, got %s\r\n", CMD_REPLY_DIFF_PORT, (char*)data);
        }
        to.detach();
        break;
    } while (1);

    // destroy the sockets
    err = api->destroy(&sock_client);
    TEST_EQ(err, SOCKET_ERROR_NONE);

    err = api->destroy(&sock_srv);
    TEST_EQ(err, SOCKET_ERROR_NONE);

test_exit:
    TEST_PRINT(">>> KILL,ES\r\n");
    free(data);
    TEST_RETURN();
}

int ns_socket_test_bind_api(socket_stack_t stack, socket_address_family_t af,
        socket_proto_family_t pf)
{
    struct socket sock_client;
    socket_error_t err;
    const struct socket_api *api = socket_get_api(stack);
    client_socket = &sock_client;

    // Create the socket
    TEST_CLEAR();
    TEST_PRINT("\r\n%s af: %d, pf: %d\r\n",  __func__, (int ) af, (int ) pf);

    if (!TEST_NEQ(api, NULL))
    {
        // Test cannot continue without API.
        TEST_RETURN();
    }
    err = api->init();
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_RETURN();
    }

    sock_client.impl = NULL;

    // Create a socket
    err = api->create(&sock_client, af, pf, &client_cb);
    if (!TEST_EQ(err, SOCKET_ERROR_NONE))
    {
        TEST_EXIT();
    }

    // test binding API

    // address not IN_ANY
    socket_addr address;
    address.storage[0] = 1;
    address.type = SOCKET_STACK_NANOSTACK_IPV6;
    err = api->bind(&sock_client, &address, CMD_REPLY_DIFF_LOCAL_PORT);
    TEST_NEQ(err, SOCKET_ERROR_NONE);
    memset(&address.storage[0], 0, 16);

    // test port 0
    address.type = SOCKET_STACK_NANOSTACK_IPV6;
    err = api->bind(&sock_client, &address, 0);
    TEST_NEQ(err, SOCKET_ERROR_NONE);

    // test address NULL
    err = api->bind(&sock_client, NULL, CMD_REPLY_DIFF_LOCAL_PORT);
    TEST_NEQ(err, SOCKET_ERROR_NONE);

    // test double binding
    err = api->bind(&sock_client, &address, CMD_REPLY_DIFF_LOCAL_PORT);
    TEST_EQ(err, SOCKET_ERROR_NONE);
    err = api->bind(&sock_client, &address, CMD_REPLY_DIFF_LOCAL_PORT);
    TEST_NEQ(err, SOCKET_ERROR_NONE);

    // destroy the socket
    err = api->destroy(&sock_client);
    TEST_EQ(err, SOCKET_ERROR_NONE);

test_exit:
    TEST_PRINT(">>> KILL,ES\r\n");
    TEST_RETURN();
}
