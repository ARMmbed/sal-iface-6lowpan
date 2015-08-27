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
/*
 * NanoStack Socket Abstraction Layer (ns_sal) test prototypes.
 * When test environment is merged these prototypes can be used from sal_test_api.h.
 */
#ifndef __TEST_CASES_H__
#define __TEST_CASES_H__

typedef void (*run_func_t)(void);

/*
 * Reused tests from mbed-net-socket-abstract/test/sal_test_api.h
 * */
int socket_api_test_create_destroy(socket_stack_t stack, socket_address_family_t disable_family);
int socket_api_test_socket_str2addr(socket_stack_t stack, socket_address_family_t disable_family);
int socket_api_test_connect_close(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t disable_family, const char *server, uint16_t port, run_func_t run_cb);
int socket_api_test_echo_client_connected(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf, bool connect,
        const char *server, uint16_t port, run_func_t run_cb, uint16_t max_packet_size);

int socket_api_test_echo_server_stream(socket_stack_t stack, socket_address_family_t af, const char *local_addr, uint16_t port, run_func_t run_cb);

/* NanoStack specific tests */

/*
 * \brief Test TCP socket binding and remote end closure.
 */
int ns_tcp_bind_and_remote_end_close(socket_stack_t stack, const char *server, uint16_t port, run_func_t run_cb, uint16_t source_port);

/*
 * \brief Test UDP received data buffering. Four datagrams buffered and client starts reading when 5th datagram arrives.
 *  -first datagram is read completely,
 *  -following datagrams are too large to fit provided buffer, datagram gets truncated
 *  -final recv_from returns error status SOCKET_ERROR_WOULD_BLOCK as no datagrams left in buffer
 */
int ns_udp_test_buffered_recv_from(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
                                   const char *server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test socket binding.
 * -Two sockets created,
 * -->first one sends command to test server to send response back to predefined port
 * -->second one is bound to predefined port that receives response from the test server.
 */
int ns_socket_test_bind(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
                        const char *server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test socket connect failure.
 * Connection attempt is made to port that doesn't have listening socket.
 */
int ns_socket_test_connect_failure(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
                                   const char *server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test maximum number of sockets.
 * -Create sockets until socket creation fails.
 * -Verify socket operation with function send_to() and recv_from()
 * -Close sockets
 */
int ns_socket_test_max_num_of_sockets(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
                                      const char *server, uint16_t port, run_func_t run_cb, uint8_t max_num_of_sockets);

/*
 * \brief Test UDP Socket data sending and receiving.
 * -Create sockets
 * -Use send_to() to send data to each socket
 * -Run stack as long as recv_from() returns data to socket
 * -Repeat sending and receiving as indicated by parameter max_loops
 * -Destroy all sockets
 */
int ns_socket_test_udp_traffic(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
                               const char *server, uint16_t port, run_func_t run_cb, uint16_t max_loops, uint8_t max_num_of_sockets);

/*
 * \brief Test recv_from API with error values.
 */
int ns_socket_test_recv_from_api(socket_stack_t stack);

/*
 * \brief Test send_to API with error values.
 */
int ns_socket_test_send_to_api(socket_stack_t stack);

/*
 * \brief Test connect API with error values.
 */
int ns_socket_test_connect_api(socket_stack_t stack);

/*
 * \brief Test socket binding API
  */
int ns_socket_test_bind_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test socket resolving API
  */
int ns_socket_test_resolve_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test socket send API
  */
int ns_socket_test_send_api(socket_stack_t stack);

/*
 * \brief Test socket send API
  */
int ns_socket_test_recv_api(socket_stack_t stack);

/*
 * \brief Test socket close API
  */
int ns_socket_test_close_api(socket_stack_t stack);

/*
 * \brief Test APIs that are unimplemented
  */
int ns_socket_test_unimplemented_apis(socket_stack_t stack);


#endif /* __TEST_CASES_H__ */

