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
#include <mbed-net-socket-abstract/test/ctest_env.h>
#include "mbed-mesh-api/Mesh6LoWPAN_ND.h"
#include "atmel-rf-driver/driverRFPhy.h"    // rf_device_register
#include "test_cases.h"
#include "mbed/test_env.h"
#define HAVE_DEBUG 1
#include "ns_trace.h"

#define TRACE_GROUP  "main"     // for traces

// main IPv6 address
// Note! Replies are coming from temporary address unless feature disabled from server side.
#define TEST_SERVER         "FD00:FF1:CE0B:A5E1:1068:AF13:9B61:D557"
#define TEST_PORT           50001
#define TEST_NO_SRV_PORT    40000   // No server on this port
#define CONNECT_PORT        50000
#define MAX_NUM_OF_SOCKETS  16      // NanoStack supports max 16 sockets, 2 are already reserved by stack..
#define STRESS_TESTS_ENABLE         // Long lasting stress tests enabled
#define STRESS_TESTS_LOOP_COUNT 100  // Stress test loop count

#define NS_MAX_UDP_PACKET_SIZE 2047

static uint8_t mesh_network_state = MESH_DISCONNECTED;
static Mesh6LoWPAN_ND *mesh_api = NULL;
static int tests_pass = 1;

int runTests(void);

void mesh_interface_run()
{
}


/*
 * Callback from mesh network. Called when network state changes.
 */
void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
    if(mesh_network_state == MESH_CONNECTED) {
        tr_info("Connected to mesh network!");
        runTests();
        mesh_api->disconnect();
    } else if (mesh_network_state == MESH_DISCONNECTED)
    {
        minar::Scheduler::stop();
        tr_info("All tests done!");
    }
}

void enable_detailed_tracing(bool high)
{
    uint8_t conf = TRACE_MODE_COLOR|TRACE_CARRIAGE_RETURN;
    if (high)
    {
        conf |= TRACE_ACTIVE_LEVEL_ALL;
    }
    else
    {
        conf |= TRACE_ACTIVE_LEVEL_INFO;
    }
    set_trace_config(conf);
}

void app_start(int, char**) {
    int8_t err;

    mesh_api = Mesh6LoWPAN_ND::getInstance();
    err = mesh_api->init(rf_device_register(), mesh_network_callback);

    if (!TEST_EQ(err, MESH_ERROR_NONE)) {
        return;
    }

    err = mesh_api->connect();
    if (!TEST_EQ(err, MESH_ERROR_NONE)) {
        return;
    }
    enable_detailed_tracing(true);
}

int runTests(void) {
    MBED_HOSTTEST_TIMEOUT(5);
    MBED_HOSTTEST_SELECT(default);
    MBED_HOSTTEST_DESCRIPTION(NanoStack Socket Abstraction Layer tests);
    MBED_HOSTTEST_START("NanoStack SAL");

    int rc;
    tests_pass = 1;

    do
    {
        rc = socket_api_test_create_destroy(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;

        rc = socket_api_test_socket_str2addr(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;

        rc = socket_api_test_connect_close(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM, TEST_SERVER, CONNECT_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;

        rc = socket_api_test_echo_client_connected(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                false, TEST_SERVER, TEST_PORT, mesh_interface_run, NS_MAX_UDP_PACKET_SIZE);
        tests_pass = tests_pass && rc;

        rc = ns_udp_test_buffered_recv_from(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                TEST_SERVER, TEST_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_bind(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                TEST_SERVER, TEST_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;

#if 0
        // Requires TCP test server... and TCP sockets doesn't work properly, skip test case
        rc = ns_socket_test_bind(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM,
                TEST_SERVER, TEST_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;
#endif
#if 0
        // TCP connect doesn't work yet, therefore can't be run
        rc = ns_socket_test_connect_failure(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM,
                TEST_SERVER, TEST_NO_SRV_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;
#endif

        rc = ns_socket_test_max_num_of_sockets(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                TEST_SERVER, TEST_PORT, mesh_interface_run, MAX_NUM_OF_SOCKETS);
        tests_pass = tests_pass && rc;

#ifdef STRESS_TESTS_ENABLE
        /*
         * Stress tests (limit traces during the test as they slow down test speed)
         */
        enable_detailed_tracing(false);
        int i;
        for (i = 0; i < STRESS_TESTS_LOOP_COUNT; i++)
        {
            rc = ns_socket_test_max_num_of_sockets(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                    TEST_SERVER, TEST_PORT, mesh_interface_run, MAX_NUM_OF_SOCKETS);
            tests_pass = tests_pass && rc;
        }

        rc = ns_socket_test_udp_traffic(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                TEST_SERVER, TEST_PORT, mesh_interface_run, STRESS_TESTS_LOOP_COUNT, 10);
        tests_pass = tests_pass && rc;
        enable_detailed_tracing(true);
#endif /* STRESS_TESTS_ENABLE */

        /*
         * Test Socket API's with illegal values
         */
        rc = ns_socket_test_recv_from_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_send_to_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_connect_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_resolve_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_bind_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
        tests_pass = tests_pass && rc;

        rc = ns_socket_test_bind_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM);
        tests_pass = tests_pass && rc;
    } while (0);

    MBED_HOSTTEST_RESULT(tests_pass);
    return !tests_pass;
}
