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
#include "mbed.h"
#include "minar/minar.h"
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
static Mesh6LoWPAN_ND *mesh_api = NULL;

/*
 * Callback from a UDPExample appl indicating data is available.
 */
void data_available_callback() {
    tr_info("data_available_callback()");
    if (udpTest->isReceived() == true) {
        //minar::Scheduler::cancelCallback(callback_handle);
        tr_info("Response received from router:");
        tr_info("Sent: %s", TEST_DATA);
        tr_info("Recv: %s", udpTest->getResponse());
        mesh_api->disconnect();
    }
}

/*
 * Callback from a mesh network. Called when network state changes.
 */
void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
    if (mesh_network_state == MESH_CONNECTED) {
        char router_addr[50];
        tr_info("Connected to mesh network!");
        if (true == mesh_api->getRouterIpAddress(router_addr, sizeof(router_addr)))
        {
            tr_info("Echoing data to router IP address: %s", router_addr);
            udpTest = new UDPTest(data_available_callback);
            udpTest->startEcho(router_addr, ECHO_PORT, TEST_DATA);
        } else {
            tr_info("Failed to read own IP address");
        }
    } else if (mesh_network_state == MESH_DISCONNECTED) {
        tr_info("Disconnected from mesh network!");
        delete udpTest;
        udpTest = NULL;
        minar::Scheduler::stop();
        tr_info("UDP echoing done!");
        tr_info("End of program.");
    } else {
        tr_error("Networking error!");
    }
}

void app_start(int, char**) {
    int8_t status;

    // init mesh api
    mesh_api = Mesh6LoWPAN_ND::getInstance();
    status = mesh_api->init(rf_device_register(), mesh_network_callback);
    if (status != MESH_ERROR_NONE) {
        tr_error("Failed to initialize mesh network, error %d!", status);
        return;
    }

    // connect to mesh network
    status = mesh_api->connect();
    if (status != MESH_ERROR_NONE) {
        tr_error("Can't connect to mesh network! error=%d", status);
        return;
    }
}
