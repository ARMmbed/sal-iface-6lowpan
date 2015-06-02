/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#include "mbed.h"
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include <mbed-net-socket-abstract/test/ctest_env.h>
#include "mbed-6lowpan-adaptor/mesh_interface.h"
#include "test_cases.h"
#include "mbed/test_env.h"

// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "main"

// main IPv6 address
// Note! Replies are coming from temporary address unless feature disabled from server side.
#define TEST_SERVER "FD00:FF1:CE0B:A5E1:1068:AF13:9B61:D557"
#define TEST_PORT           50001
#define TEST_NO_SRV_PORT    40000   // No server on this port
#define CONNECT_PORT        50000

#define NS_MAX_UDP_PACKET_SIZE 2047

volatile uint8_t network_ready = 0;

void nanostack_network_ready(void)
{
    tr_info("NanoStack network ready, start testing...\r\n");
    network_ready = 1;
}

int main()
{
    MBED_HOSTTEST_TIMEOUT(5);
    MBED_HOSTTEST_SELECT(default);
    MBED_HOSTTEST_DESCRIPTION(NanoStack Socket Abstraction Layer tests);
    MBED_HOSTTEST_START("NanoStack SAL");

    int rc;
    int tests_pass = 1;

    do
    {
        socket_error_t err = mesh_interface_init();
        if (!TEST_EQ(err, SOCKET_ERROR_NONE))
        {
            break;
        }

        err = mesh_interface_connect(nanostack_network_ready);
        if (!TEST_EQ(err, SOCKET_ERROR_NONE))
        {
            break;
        }

        do
        {
            mesh_interface_run();
        } while (0 == network_ready);

        rc = socket_api_test_create_destroy(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;

        rc = socket_api_test_socket_str2addr(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET4);
        tests_pass = tests_pass && rc;

#if 0
        // close doesn't generate callback and test case fails = > will be fixed when making connect/close
        rc = socket_api_test_connect_close(SOCKET_STACK_NANOSTACK_IPV6, SOCKET_AF_INET6, SOCKET_DGRAM, TEST_SERVER, CONNECT_PORT, mesh_interface_run);
        tests_pass = tests_pass && rc;
#endif

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

        /* Test APIs with illegal values */
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
