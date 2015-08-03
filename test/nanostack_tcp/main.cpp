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
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-sockets/TCPStream.h>
#include "atmel-rf-driver/driverRFPhy.h"    // rf_device_register
#include "mbed-mesh-api/Mesh6LoWPAN_ND.h"
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
static uint8_t mesh_network_state = MESH_DISCONNECTED;
static Mesh6LoWPAN_ND *mesh_api = NULL;

void data_available_callback() {
    tr_info("data_available_callback()");
    if (tcpExample->isReceived() == true) {
        tr_info("Received data from server:");
        tr_info("Recv: %s", tcpExample->getResponse());
        tr_info("Sent: %s", TEST_DATA);
        delete tcpExample;
        tcpExample = NULL;
        mesh_api->disconnect();
    }
}

void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
    if (mesh_network_state == MESH_CONNECTED) {
        tr_info("Connected to mesh network!");

        tcpExample = new TCPExample(data_available_callback);
        tcpExample->startEcho(HOST_ADDR, TEST_PORT, TEST_DATA, sizeof(TEST_DATA));
    } else if (mesh_network_state == MESH_DISCONNECTED) {
        delete tcpExample;
        tcpExample = NULL;
        minar::Scheduler::stop();
        tr_info("TCP echoing done!");
        tr_info("End of program.");
    } else {
        tr_error("bad network state");
    }

}

void app_start(int, char**) {
    int8_t status;

    mesh_api = Mesh6LoWPAN_ND::getInstance();
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

    uint8_t trace_conf = TRACE_MODE_COLOR|TRACE_CARRIAGE_RETURN|TRACE_ACTIVE_LEVEL_ALL;
    set_trace_config(trace_conf);
}
