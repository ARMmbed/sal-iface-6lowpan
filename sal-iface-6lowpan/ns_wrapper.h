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
 * NanoStack wrapper module, used to hide NanoStack internals from
 * socket abstraction layer (SAL).
 *
 * Additional layer between SAL and NanoStack socket API was needed
 * to avoid function naming conflicts.
 *
 */
#ifndef NANOSTACK_SOCKET_IMPL_H
#define NANOSTACK_SOCKET_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/* NanoStack socket types */
#define NANOSTACK_SOCKET_UDP 17 // same as nanostack SOCKET_UDP
#define NANOSTACK_SOCKET_TCP 6  // same as nanostack SOCKET_TCP

/* typedef for function pointer parameter */
typedef void (*func_cb_t)(void);

/*
 * Socket data attached to mbed socket structure.
 */
typedef struct sock_data_ {
    int8_t socket_id;           /*!< allocated socket ID */
    int8_t security_session_id; /*!< Not used yet */
} sock_data_s;

/*
 * Nanostack wrapper functions
 */

/*
 * \brief Open NanoStack socket
 */
sock_data_s *ns_wrapper_socket_open(int8_t socket_type, int8_t identifier, void *context);

/*
 * \brief Bind NanoStack socket
 */
int8_t ns_wrapper_socket_bind(sock_data_s *sock_data_ptr, ns_address_t *address);

/*
 * \brief Release data allocated by NanoStack socket
 */
void ns_wrapper_release_socket_data(sock_data_s *sock_data_ptr);

/*
 * \brief Free resources allocated by NanoStack socket
 */
int8_t ns_wrapper_socket_free(sock_data_s *sock_data_ptr);

/*
 * \brief Close NanoStack socket
 */
int8_t ns_wrapper_socket_close(sock_data_s *sock_data_ptr);

/*
 * \brief Connect NanoStack socket
 */
int8_t ns_wrapper_socket_connect(sock_data_s *sock_data_ptr, ns_address_t *address);

/*
 * \brief Send data to NanoStack socket
 */
int8_t ns_wrapper_socket_send(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length);

/*
 * \brief Send data to NanoStack socket
 */
int8_t ns_wrapper_socket_send_to(sock_data_s *sock_data_ptr, ns_address_t *address, uint8_t *buffer, uint16_t length);

/*
 * \brief Receive data from NanoStack socket
 */
int8_t ns_wrapper_socket_recv_from(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif
#endif
