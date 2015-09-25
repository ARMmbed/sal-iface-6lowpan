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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "nsdynmemLIB.h"
#include "ip6string.h"  //ip6tos
#include "ns_address.h"
#include "net_interface.h"
#include "socket_api.h" // nanostack socket api
#define HAVE_DEBUG 1
#include "ns_trace.h"
#include "sal-iface-6lowpan/ns_sal_callback.h"
#include "sal-iface-6lowpan/ns_wrapper.h"

// For tracing we need to define define group
#define TRACE_GROUP  "ns_wrap"

/* function entry traces */
#define FUNC_ENTRY_TRACE_ENABLED
#ifdef FUNC_ENTRY_TRACE_ENABLED
#define FUNC_ENTRY_TRACE    tr_debug
#else
#define FUNC_ENTRY_TRACE(...)
#endif

#define NS_WRAPPER_SOCKETS_MAX  16  //same as NanoStack SOCKET_MAX
#define MALLOC  ns_dyn_mem_alloc
#define FREE    ns_dyn_mem_free

// table for socket contexts
typedef struct _socket_context_map_t {
    void *context;
} socket_context_map_t;

// table of mbed sockets
static socket_context_map_t socket_context_tbl[NS_WRAPPER_SOCKETS_MAX] = {{0}};

/**** Private functions ****/

/*
 * Handler for the received data
 */
void ns_wrapper_data_received(socket_callback_t *sock_cb)
{
    if (sock_cb->d_len > 0) {
        data_buff_t *recv_buff = (data_buff_t *) ns_dyn_mem_alloc(
                                     sizeof(data_buff_t) + sock_cb->d_len);
        if (NULL != recv_buff) {
            int16_t length = socket_read(sock_cb->socket_id,
                                         &recv_buff->ns_address, recv_buff->payload,
                                         sock_cb->d_len);
            recv_buff->length = length;
            recv_buff->next = NULL;

            ns_sal_callback_data_received(socket_context_tbl[sock_cb->socket_id].context, recv_buff);
            // allocated memory will be deallocated when application reads the data or when socket is closed
        }
    } else {
        ns_sal_callback_data_received(socket_context_tbl[sock_cb->socket_id].context, NULL);
    }
}

/*
 * Socket callback, called automatically by the NanoStack when event occurs.
 */
void ns_wrapper_socket_callback(void *cb)
{
    socket_callback_t *sock_cb = (socket_callback_t *) cb;

    FUNC_ENTRY_TRACE("socket_callback() sock=%d, event=%d, interface=%d, data len=%d",
                     sock_cb->socket_id, sock_cb->event_type, sock_cb->interface_id, sock_cb->d_len);

    switch (sock_cb->event_type) {
        case SOCKET_DATA:
            tr_debug("SOCKET_DATA, sock=%d, bytes=%d", sock_cb->socket_id, sock_cb->d_len);
            ns_wrapper_data_received(sock_cb);
            break;
        case SOCKET_BIND_DONE:
            tr_debug("SOCKET_BIND_DONE");
            ns_sal_callback_connect(socket_context_tbl[sock_cb->socket_id].context);
            break;
        case SOCKET_BIND_FAIL:
            tr_debug("SOCKET_BIND_FAIL");
            break;
        case SOCKET_BIND_AUTH_FAIL:
            tr_debug("SOCKET_BIND_AUTH_FAIL");
            break;
        case SOCKET_INCOMING_CONNECTION:
            tr_debug("SOCKET_INCOMING_CONNECTION");
            ns_sal_callback_pending_connection(socket_context_tbl[sock_cb->socket_id].context);
            break;
        case SOCKET_TX_FAIL:
            tr_debug("SOCKET_TX_FAIL");
            break;
        case SOCKET_CONNECT_CLOSED:
            tr_debug("SOCKET_CONNECT_CLOSED");
            ns_sal_callback_disconnect(socket_context_tbl[sock_cb->socket_id].context);
            break;
        case SOCKET_CONNECTION_RESET:
            tr_debug("SOCKET_CONNECTION_RESET");
            break;
        case SOCKET_NO_ROUTE:
            tr_debug("SOCKET_NO_ROUTE");
            break;
        case SOCKET_TX_DONE:
            tr_debug("SOCKET_TX_DONE, %d bytes sent", sock_cb->d_len);
            ns_sal_callback_tx_done(socket_context_tbl[sock_cb->socket_id].context, sock_cb->d_len);
            break;

        default:
            // SOCKET_NO_RAM, error case for SOCKET_TX_DONE
            ns_sal_callback_tx_error(socket_context_tbl[sock_cb->socket_id].context);
            break;
    }
}

void ns_wrapper_release_socket_data(sock_data_s *sock_data_ptr)
{
    FREE(sock_data_ptr);
}

int8_t ns_wrapper_socket_free(sock_data_s *sock_data_ptr)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_free() sock=%d", sock_data_ptr->socket_id);
    int8_t retval = socket_close(sock_data_ptr->socket_id);
    ns_wrapper_release_socket_data(sock_data_ptr);
    return retval;
}

sock_data_s * ns_wrapper_socket_validate(sock_data_s *sock_data_ptr,  void *context)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_validate() sock=%d", sock_data_ptr->socket_id);
    if ((sock_data_ptr->socket_id >= 0) &&
            (sock_data_ptr->socket_id < NS_WRAPPER_SOCKETS_MAX)) {
        sock_data_ptr->security_session_id = 0;
        sock_data_ptr->flags = 0;
        // save context to table so that callbacks can be made to right socket
        socket_context_tbl[sock_data_ptr->socket_id].context = context;
    } else {
        /* socket opening failed, free reserved data */
        tr_error("invalid socket id");
        FREE(sock_data_ptr);
        sock_data_ptr = NULL;
    }

    return sock_data_ptr;
}

sock_data_s *ns_wrapper_socket_open(int8_t socket_type, int8_t identifier, void *context)
{
    int8_t protocol = SOCKET_TCP;
    FUNC_ENTRY_TRACE("ns_wrapper_socket_open()");
    if (NANOSTACK_SOCKET_UDP == socket_type) {
        protocol = SOCKET_UDP;
    }

    sock_data_s *sock_data_ptr = (sock_data_s *) MALLOC(sizeof(sock_data_s));
    if (NULL != sock_data_ptr) {
        sock_data_ptr->socket_id = socket_open(protocol, identifier,
                                               ns_wrapper_socket_callback);
        sock_data_ptr = ns_wrapper_socket_validate(sock_data_ptr, context);
    }

    return sock_data_ptr;
}

int8_t ns_wrapper_socket_bind(sock_data_s *sock_data_ptr, ns_address_t *address)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_bind() sock=%d", sock_data_ptr->socket_id);
    return socket_bind(sock_data_ptr->socket_id, address);
}

int8_t ns_wrapper_socket_shutdown(sock_data_s *sock_data_ptr)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_close() sock=%d", sock_data_ptr->socket_id);
    int8_t error = socket_shutdown(sock_data_ptr->socket_id, SOCKET_SHUT_RDWR);
    return error;
}

int8_t ns_wrapper_socket_connect(sock_data_s *sock_data_ptr, ns_address_t *address)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_connect() sock=%d", sock_data_ptr->socket_id);
    return socket_connect(sock_data_ptr->socket_id, address, 0);
}

int8_t ns_wrapper_socket_send(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_send: sock=%d, length=%d", sock_data_ptr->socket_id, length);
    return socket_send(sock_data_ptr->socket_id, buffer, length);
}

int8_t ns_wrapper_socket_send_to(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_send_to: sock=%d, length=%d, port=%d", sock_data_ptr->socket_id, length, addr->identifier);
    return socket_sendto(sock_data_ptr->socket_id, addr, buffer, length);
}

int8_t ns_wrapper_socket_listen(sock_data_s *sock_data_ptr, uint8_t backlog)
{
    FUNC_ENTRY_TRACE("ns_wrapper_socket_listen: sock=%d, backlog=%d", sock_data_ptr->socket_id, backlog);
    return socket_listen(sock_data_ptr->socket_id, backlog);
}

sock_data_s *ns_wrapper_socket_accept(int8_t listen_socket_id, void *context)
{
    ns_address_t addr;
    sock_data_s *sock_data_ptr = (sock_data_s *) MALLOC(sizeof(sock_data_s));
    if (sock_data_ptr) {
        sock_data_ptr->socket_id = socket_accept(listen_socket_id, &addr, ns_wrapper_socket_callback);
        sock_data_ptr = ns_wrapper_socket_validate(sock_data_ptr, context);
    }

    return sock_data_ptr;
}
