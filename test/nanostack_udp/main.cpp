/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#include "mbed.h"
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include "test_env.h"
#include "mbed-mesh-api/Mesh6LoWPAN_ND.h"
#include "atmel-rf-driver/driverRFPhy.h"    // rf_device_register
#include "UDPTest.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "main_appl"

#define ECHO_PORT   7
#define TEST_DATA   "Example UDP data message sent to gateway and echoed back!"

static UDPTest *udpTest = NULL;
static mesh_connection_status_t mesh_network_state = MESH_DISCONNECTED;

/*
 * Callback from mesh network. Called when network state changes.
 */
void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
}

int main() {
    Mesh6LoWPAN_ND *mesh_api = Mesh6LoWPAN_ND::getInstance();
    char router_addr[50];
    int8_t status;

    status = mesh_api->init(rf_device_register(), mesh_network_callback);
    if (status != MESH_ERROR_NONE)
    {
        tr_error("Mesh network initialization failed %d!", status);
        return 1;
    }

    status = mesh_api->connect();

    if (status != MESH_ERROR_NONE)
    {
        tr_error("Can't connect to mesh network!");
        return 1;
    }

    do
    {
        mesh_api->processEvent();
    } while(mesh_network_state != MESH_CONNECTED);

    tr_info("NanoStack connected to mesh network");

    tr_info("Start UDPTest");

    if (true == mesh_api->getRouterIpAddress(router_addr, sizeof(router_addr)))
    {
        tr_info("Echoing data to router IP address: %s", router_addr);
        udpTest = new UDPTest();
        udpTest->startEcho(router_addr, ECHO_PORT, TEST_DATA);
    } else {
        tr_info("Failed to read own IP address");
        return -1;
    }

    /* wait network to be established */
    do {
        mesh_api->processEvent();
    } while(0 == udpTest->isReceived());

    tr_info("Response received from router:");
    tr_info("Sent: %s", TEST_DATA);
    tr_info("Recv: %s", udpTest->getResponse());
    delete udpTest;
    udpTest = NULL;

    mesh_api->disconnect();
    do
    {
        mesh_api->processEvent();
    } while(mesh_network_state != MESH_DISCONNECTED);

    tr_info("UDP echoing done!");

    return 0;
}
