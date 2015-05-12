/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#include "mbed.h"
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include "test_env.h"

// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "main_appl"

#include "mbed-6lowpan-adaptor/nanostack_init.h"
#include "UDPTest.h"

#define ECHO_PORT   7
#define TEST_DATA   "Example UDP data message sent to gateway and echoed back!"

static UDPTest *udpTest = NULL;
volatile uint8_t network_ready = 0;

void nanostack_network_ready(void)
{
    tr_info("Network established");
    network_ready = 1;
}

int main() {
    socket_error_t error_status;
    char router_addr[50];

    error_status = nanostack_init();
    if (SOCKET_ERROR_NONE != error_status)
    {
        tr_error("Can't initialize NanoStack!");
        return 1;
    }
    tr_info("NanoStack UDP Example, stack initialized");

    error_status = nanostack_connect(nanostack_network_ready);

    if (SOCKET_ERROR_NONE != error_status)
    {
        tr_error("Can't connect to NanoStack!");
        return 1;
    }

    do
    {
        nanostack_run();
    } while(0 == network_ready);

    tr_info("NanoStack connected to network");

    tr_info("Start UDPTest");
    if (SOCKET_ERROR_NONE == nanostack_get_router_ip_address(router_addr, sizeof(router_addr)))
    {
        tr_info("Echoing data to router IP address: %s", router_addr);
        udpTest = new UDPTest();
        udpTest->startEcho(router_addr, ECHO_PORT, TEST_DATA);
    }

    /* wait network to be established */
    do {
        nanostack_run();
    } while(0 == udpTest->isReceived());

    tr_info("Response received from router:");
    tr_info("Sent: %s", TEST_DATA);
    tr_info("Recv: %s", udpTest->getResponse());
    delete udpTest;
    udpTest = NULL;

    nanostack_disconnect();
//    do
//    {
//        nanostack_run();
//        __WFI();
//    } while(0 == disconnected);

    tr_info("All done!");

    return 0;
}
