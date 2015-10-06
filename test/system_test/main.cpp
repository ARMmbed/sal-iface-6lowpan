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
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-socket-abstract/test/ctest_env.h>
#include "mbed-mesh-api/Mesh6LoWPAN_ND.h"
#include "mbed-mesh-api/MeshInterfaceFactory.h"
#include "atmel-rf-driver/driverRFPhy.h"    // rf_device_register
#include "test_cases.h"
#include "mbed/test_env.h"
#include "core-util/FunctionPointer.h"
#include "eventOS_scheduler.h"
#define HAVE_DEBUG 1
#include "ns_trace.h"

#define TRACE_GROUP  "main"     // for traces

using mbed::util::FunctionPointer0;

// main IPv6 address
// Note! Replies are coming from temporary address unless feature is disabled from server side.
#define TEST_SERVER         "FD00:FF1:CE0B:A5E0:21B:38FF:FE90:ABB1"
#define TEST_PORT           50001   // UDP server listening on this port
#define TEST_NO_SRV_PORT    40000   // No server listening on this port
#define TCP_PORT            50000   // TCP server listening on this port
#define CONNECT_SOURCE_PORT 55555   // TCP socket bound port

#define MAX_NUM_OF_SOCKETS  16      // NanoStack supports max 16 sockets, 2 are already reserved by stack.
#define STRESS_TESTS_LOOP_COUNT 100 // Stress test loop count

#define NS_MAX_UDP_PACKET_SIZE 2047
#define NS_MAX_TCP_PACKET_SIZE 4096

static mesh_connection_status_t mesh_network_state = MESH_DISCONNECTED;
static Mesh6LoWPAN_ND *mesh_api = NULL;
static int tests_pass = 1;

using namespace minar;

void schedule_test_execution(int delay_ms);
int runTest(int testID);
int runAPITest(int testID);
int runStressTest(int testID);

class TestExecutor;
TestExecutor *testExecutor;

class TestExecutor
{
public:

    TestExecutor() : loops(1), testRound(1), testID(0), testIDAPI(0), testIDStress(0)
    {}

    void executeTests(void) {
        if (runTest(testID) != -1) {
            testID++;
            schedule_test_execution(1000);
            return;
        } else if (runAPITest(testIDAPI) != -1) {
            testIDAPI++;
            schedule_test_execution(1000);
            return;
        } else if (runStressTest(testIDStress) != -1) {
            testIDStress++;
            schedule_test_execution(1000);
            return;
        }

        if (loops == 1) {
            mesh_api->disconnect();
        } else {
            loops--;
            testRound++;
            if (tests_pass == 1) {
                printf("\r\n#####\r\n");
                printf("\r\nStart test round %d, current result=%d\r\n", testRound, tests_pass);
                printf("\r\n#####\r\n");
                testID = testIDAPI = testIDStress = 0;
                schedule_test_execution(30000);
            } else
            {
                printf("\r\nTests FAILED! Testing stopped.\r\n");
                mesh_api->disconnect();
            }
        }
    }

    // number of test loops, to execute all tests once set to 1
    int loops;
    // current test round
    int testRound;
    // test ID
    int testID;
    // test ID in API test set
    int testIDAPI;
    // test ID in stress test set
    int testIDStress;
};

void schedule_test_execution(int delay_ms)
{
    Scheduler::postCallback(FunctionPointer0<void>(testExecutor, &TestExecutor::executeTests).bind()).delay(minar::milliseconds(delay_ms));
}

void mesh_process_events(void)
{
    eventOS_scheduler_dispatch_event();
}

/*
 * Callback from mesh network. Called when network state changes.
 */
void mesh_network_callback(mesh_connection_status_t mesh_state)
{
    tr_info("mesh_network_callback() %d", mesh_state);
    mesh_network_state = mesh_state;
    if (mesh_network_state == MESH_CONNECTED) {
        tr_info("Connected to mesh network!");
        schedule_test_execution(0);
    } else if (mesh_network_state == MESH_DISCONNECTED) {
        tr_info("All tests done!");
        MBED_HOSTTEST_RESULT(tests_pass);
    }
}

void enable_detailed_tracing(bool high)
{
    uint8_t conf = TRACE_MODE_COLOR | TRACE_CARRIAGE_RETURN;
    if (high) {
        conf |= TRACE_ACTIVE_LEVEL_ALL;
    } else {
        conf |= TRACE_ACTIVE_LEVEL_INFO;
    }
    set_trace_config(conf);
}

void app_start(int, char **)
{
    MBED_HOSTTEST_TIMEOUT(5);
    MBED_HOSTTEST_SELECT(default);
    MBED_HOSTTEST_DESCRIPTION(6LoWPAN Socket Abstraction Layer tests);
    MBED_HOSTTEST_START("6LoWPAN Socket Abstraction Layer");

    mesh_error_t err;
    tests_pass = 1;

    // set tracing baud rate
    static Serial pc(USBTX, USBRX);
    pc.baud(115200);

    testExecutor = new TestExecutor;

    mesh_api = (Mesh6LoWPAN_ND *)MeshInterfaceFactory::createInterface(MESH_TYPE_6LOWPAN_ND);
    err = mesh_api->init(rf_device_register(), mesh_network_callback);

    if (!TEST_EQ(err, MESH_ERROR_NONE)) {
        return;
    }

    err = mesh_api->connect();
    if (!TEST_EQ(err, MESH_ERROR_NONE)) {
        return;
    }
    enable_detailed_tracing(true);

    /* Tests will be started from callback once connected to the network */
}

/**
 * Mesh network tests
 * return -1 if no more tests needs to be run in this set.
 */
int runTest(int testID)
{
    int rc;
    int retVal = 0;

    switch(testID)
    {
    case 0:
        rc = socket_api_test_create_destroy(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;
        break;
    case 1:
        rc = socket_api_test_socket_str2addr(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;
        break;
    case 2:
        rc = socket_api_test_connect_close(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM, TEST_SERVER, TCP_PORT, mesh_process_events);
        tests_pass = tests_pass && rc;
        break;
    case 3:
        rc = socket_api_test_echo_client_connected(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                false, TEST_SERVER, TEST_PORT, mesh_process_events, NS_MAX_UDP_PACKET_SIZE);
        tests_pass = tests_pass && rc;
        break;
    case 4:
        rc = socket_api_test_echo_client_connected(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM,
                true, TEST_SERVER, TCP_PORT, mesh_process_events, NS_MAX_TCP_PACKET_SIZE);
        tests_pass = tests_pass && rc;
        break;
    case 5:
        rc = ns_tcp_bind_and_remote_end_close(SOCKET_STACK_NANOSTACK_IPV6, TEST_SERVER, TCP_PORT, mesh_process_events, CONNECT_SOURCE_PORT);
        tests_pass = tests_pass && rc;
        break;
    case 6:
        rc = ns_udp_test_buffered_recv_from(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                                            TEST_SERVER, TEST_PORT, mesh_process_events);
        tests_pass = tests_pass && rc;
        break;
    case 7:
        rc = ns_socket_test_bind(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                                 TEST_SERVER, TEST_PORT, mesh_process_events);
        tests_pass = tests_pass && rc;
        break;
    case 8:
        rc = ns_socket_test_connect_failure(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM,
                                            TEST_SERVER, TEST_NO_SRV_PORT, mesh_process_events);
        tests_pass = tests_pass && rc;
        break;
    default:
        retVal = -1;
        break;
    }

    //
    return retVal;
}

/**
 * API test cases
 * return -1 if no more tests needs to be run in this set.
 */
int runAPITest(int testID)
{
    (void)testID;
    int rc;

    rc = ns_socket_test_recv_from_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_send_to_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_connect_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_resolve_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_bind_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_bind_api(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_STREAM);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_send_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_recv_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_close_api(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    rc = ns_socket_test_unimplemented_apis(SOCKET_STACK_NANOSTACK_IPV6);
    tests_pass = tests_pass && rc;

    return -1; // no more tests to run in this set
}

/**
 * Stress test cases
 * return -1 if no more tests needs to be run in this set.
 */
int runStressTest(int testID)
{
    (void)testID;
    int rc;

    rc = ns_socket_test_max_num_of_sockets(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                                           TEST_SERVER, TEST_PORT, mesh_process_events, MAX_NUM_OF_SOCKETS);
    tests_pass = tests_pass && rc;

    /*
     * Stress tests (limit traces during the test as they slow down test speed)
     */
    enable_detailed_tracing(false);
    int i;
    for (i = 0; i < STRESS_TESTS_LOOP_COUNT; i++) {
        rc = ns_socket_test_max_num_of_sockets(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
                TEST_SERVER, TEST_PORT, mesh_process_events, MAX_NUM_OF_SOCKETS);
        tests_pass = tests_pass && rc;
    }

    rc = ns_socket_test_udp_traffic(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM,
            TEST_SERVER, TEST_PORT, mesh_process_events, STRESS_TESTS_LOOP_COUNT, 10);
    tests_pass = tests_pass && rc;
    enable_detailed_tracing(true);

    return -1;
}
