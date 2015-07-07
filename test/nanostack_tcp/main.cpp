/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#include "mbed.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-sockets/TCPStream.h>
#include "../../mbed-6lowpan-adaptor/mesh_interface.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "main_appl"

#include "TCPExample.h"

// Remote server port and address
#define TEST_PORT   50000
#define HOST_ADDR   "FD00:FF1:CE0B:A5E1:1068:AF13:9B61:D557"

// data to be sent and echoed
#define TEST_DATA   "Example TCP data sent to server that echoes it back!"

static TCPExample *tcpExample = NULL;
volatile uint8_t network_ready = 0;

void nanostack_network_ready(void)
{
    tr_info("Network established");
    network_ready = 1;
}

int main() {
    socket_error_t error_status;

    error_status = mesh_interface_init();
    if (SOCKET_ERROR_NONE != error_status)
    {
        tr_error("Can't initialize NanoStack!");
        return 1;
    }
    tr_info("NanoStack TCP Example, stack initialized");

    error_status = mesh_interface_connect(nanostack_network_ready);

    if (SOCKET_ERROR_NONE != error_status)
    {
        tr_error("Can't connect to NanoStack!");
        return 1;
    }

    uint8_t trace_conf = TRACE_MODE_COLOR|TRACE_CARRIAGE_RETURN|TRACE_ACTIVE_LEVEL_ALL;
    set_trace_config(trace_conf);

    /* wait network to be established */
    do
    {
        mesh_interface_run();
    } while(0 == network_ready);

    tr_info("NanoStack connected to network");

    tr_info("Start TCPExample");
    tcpExample = new TCPExample();
    tcpExample->startEcho(HOST_ADDR, TEST_PORT, TEST_DATA, sizeof(TEST_DATA));

    /* wait remote server response */
    do {
        mesh_interface_run();
    } while(0 == tcpExample->isReceived());

    tr_info("Received data from server:");
    tr_info("Recv: %s", tcpExample->getResponse());
    tr_info("Sent: %s", TEST_DATA);
    delete tcpExample;
    tcpExample = NULL;

    mesh_interface_disconnect();
//    do
//    {
//        nanostack_run();
//        __WFI();
//    } while(0 == disconnected);

    tr_info("All done!");

    return 0;
}
