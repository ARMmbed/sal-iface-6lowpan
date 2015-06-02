/*
 * Copyright (c) 2015 ARM. All rights reserved.
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
int socket_api_test_connect_close(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t disable_family, const char* server, uint16_t port, run_func_t run_cb);
int socket_api_test_echo_client_connected(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf, bool connect,
        const char* server, uint16_t port, run_func_t run_cb, uint16_t max_packet_size);

/* NanoStack specific tests */
/*
 * \brief Test UDP received data buffering. Four datagrams buffered and client starts reading when 5th datagram arrives.
 *  -first datagram is read completely,
 *  -following datagrams are too large to fit provided buffer, datagram gets truncated
 *  -final recv_from returns error status SOCKET_ERROR_WOULD_BLOCK as no datagrams left in buffer
 */
int ns_udp_test_buffered_recv_from(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
        const char* server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test socket binding.
 * -Two sockets created,
 * -->first one sends command to test server to send response back to predefined port
 * -->second one is bound to predefined port that receives response from the test server.
 */
int ns_socket_test_bind(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
        const char* server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test socket connect failure.
 * Connection attempt is made to port that doesn't have listening socket.
 */
int ns_socket_test_connect_failure(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf,
        const char* server, uint16_t port, run_func_t run_cb);

/*
 * \brief Test recv_from API with error values.
 */
int ns_socket_test_recv_from_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test send_to API with error values.
 */
int ns_socket_test_send_to_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test connect API with error values.
 */
int ns_socket_test_connect_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test socket binding API
  */
int ns_socket_test_bind_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);

/*
 * \brief Test socket resolving API
  */
int ns_socket_test_resolve_api(socket_stack_t stack, socket_address_family_t af, socket_proto_family_t pf);


#endif /* __TEST_CASES_H__ */

