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
 * NanoStack adaptation to mbed socket API.
 */

#include <string.h> // memcpy
#include <mbed-net-socket-abstract/socket_api.h>
#include "ns_address.h"
#include "net_interface.h"
#include "ip6string.h"  //stoip6
#include "sal-iface-6lowpan/ns_sal_callback.h"
#include "sal-iface-6lowpan/ns_sal_utils.h"
#include "sal-iface-6lowpan/ns_wrapper.h"
#include "common_functions.h"
#include "nsdynmemLIB.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "ns_sal"

#define MALLOC  ns_dyn_mem_alloc
#define FREE    ns_dyn_mem_free

//#define FUNC_ENTRY_TRACE_ENABLED
#ifdef FUNC_ENTRY_TRACE_ENABLED
#define FUNC_ENTRY_TRACE    tr_debug
#else
#define FUNC_ENTRY_TRACE(...)
#endif


// Forward declaration of this socket_api
const struct socket_api nanostack_socket_api;

/*** PUBLIC METHODS ***/
/*
 * \brief NanoStack initialization method. Called by application.
 */
socket_error_t ns_sal_init_stack(void)
{
    return socket_register_stack(&nanostack_socket_api);
}

/*** PRIVATE METHODS ***/
void ns_sal_copy_datagrams(struct socket *socket, uint8_t *dest, size_t *len,
                           struct socket_addr *addr, uint16_t *port)
{
    data_buff_t *data_buf = (data_buff_t *) socket->rxBufChain;

    convert_ns_addr_to_mbed(addr, &data_buf->ns_address, port);

    if ((data_buf->length) > *len) {
        /* Partial copy, more data than space avail */
        data_buf->length = *len;
    }

    memcpy(dest, data_buf->payload, data_buf->length);
    *len = data_buf->length;
    socket->rxBufChain = data_buf->next;
    FREE(data_buf);
}

void ns_sal_copy_stream(struct socket *socket, uint8_t *dest, size_t *len)
{
    uint16_t copied_total = 0;
    data_buff_t *data_buf = (data_buff_t *) socket->rxBufChain;

    for (; *len != 0 && NULL != data_buf;) {
        if ((data_buf->length + copied_total) > *len) {
            /* Partial copy, more data than space avail */
            uint16_t partial_amount = *len - copied_total;
            if (0 != partial_amount) {
                memcpy(&dest[copied_total], &data_buf->payload, partial_amount);
                copied_total += partial_amount;
                /* move memory to start of payload and adjust length */
                memmove(&data_buf->payload[0],
                        &data_buf->payload[partial_amount],
                        data_buf->length - partial_amount);
                data_buf->length -= partial_amount;
            }
            break;
        } else {
            /* Full copy, copy whole buffer to dest and move next one to first */
            memcpy(&dest[copied_total], data_buf->payload, data_buf->length);
            copied_total += data_buf->length;
            socket->rxBufChain = data_buf->next;
            FREE(data_buf);
            data_buf = (data_buff_t *)socket->rxBufChain;
        }
    } /* for space avail and data available */

    *len = copied_total;
}

socket_error_t ns_sal_recv_validate(struct socket *socket, void *buf, size_t *len)
{
    if (socket == NULL || len == NULL || buf == NULL || socket->impl == NULL) {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (*len == 0) {
        return SOCKET_ERROR_SIZE;
    }

    if (NULL == socket->rxBufChain) {
        return SOCKET_ERROR_WOULD_BLOCK;
    }

    return SOCKET_ERROR_NONE;
}

/*** SOCKET API METHODS ***/

/* socket_api function, see socket_api.h for details */
static socket_error_t ns_sal_init()
{
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t ns_sal_socket_create(struct socket *sock,
        const socket_address_family_t af, const socket_proto_family_t pf,
        socket_api_handler_t const handler)
{
    sock_data_s *sock_data_ptr;

    FUNC_ENTRY_TRACE("ns_sal_socket_create() af=%d pf=%d", af, pf);

    if (NULL == sock || NULL == handler) {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (SOCKET_AF_INET6 != af) {
        // Only ipv6 is supported by this stack
        return SOCKET_ERROR_BAD_FAMILY;
    }

    sock->stack = SOCKET_STACK_NANOSTACK_IPV6;
    switch (pf) {
        case SOCKET_DGRAM:
            sock_data_ptr = ns_wrapper_socket_open(NANOSTACK_SOCKET_UDP, 0,
                                                   (void *) sock);
            break;
        case SOCKET_STREAM:
            sock_data_ptr = ns_wrapper_socket_open(NANOSTACK_SOCKET_TCP, 0,
                                                   (void *) sock);
            break;
        default:
            return SOCKET_ERROR_BAD_FAMILY;
    }

    if (NULL == sock_data_ptr) {
        return SOCKET_ERROR_BAD_ALLOC;
    } else if (-1 == sock_data_ptr->socket_id) {
        ns_wrapper_release_socket_data(sock_data_ptr);
        return SOCKET_ERROR_UNKNOWN;
    }
    sock->impl = sock_data_ptr;
    sock->family = pf;
    sock->handler = (void *) handler;
    sock->rxBufChain = NULL;
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t ns_sal_socket_destroy(struct socket *sock)
{
    socket_error_t err = SOCKET_ERROR_NONE;
    FUNC_ENTRY_TRACE("ns_sal_socket_destroy()");
    if (NULL == sock) {
        return SOCKET_ERROR_NULL_PTR;
    }

    data_buff_t *data_buf = (data_buff_t *)sock->rxBufChain;
    while (NULL != data_buf) {
        data_buff_t *tmp_buf = data_buf;
        data_buf = data_buf->next;
        FREE(tmp_buf);
    }

    sock->rxBufChain = NULL;

    if (NULL != sock->impl) {
        int8_t status = ns_wrapper_socket_free(sock->impl);
        sock->impl = NULL;
        if (0 != status) {
            err = SOCKET_ERROR_UNKNOWN;
        }
    }

    return err;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t ns_sal_socket_close(struct socket *sock)
{
    socket_error_t error = SOCKET_ERROR_UNKNOWN;
    int8_t return_value;
    if (NULL == sock || NULL == sock->impl) {
        return SOCKET_ERROR_NULL_PTR;
    }

    return_value = ns_wrapper_socket_close(sock->impl);

    switch (return_value) {
        case 0:
            error = SOCKET_ERROR_NONE;
            break;
        case -1:
            // -1 if a given socket ID is not found, if a socket type is wrong or tcp_close() returns a failure.
            error = SOCKET_ERROR_BAD_FAMILY;
            break;
        case -2:
        case -3:
            error = SOCKET_ERROR_NO_CONNECTION;
            break;
        default:
            error = SOCKET_ERROR_UNKNOWN;
    }
    return error;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_connect(struct socket *sock,
                                     const struct socket_addr *address, const uint16_t port)
{
    socket_error_t error_code;
    if (NULL == sock || NULL == address || NULL == sock->impl) {
        return SOCKET_ERROR_NULL_PTR;
    }

    ns_address_t ns_address;
    convert_mbed_addr_to_ns(&ns_address, address, port);
    switch (ns_wrapper_socket_connect(sock->impl, &ns_address)) {
        case 0:
            error_code = SOCKET_ERROR_NONE;
            break;
        case -1:
            error_code = SOCKET_ERROR_BAD_ARGUMENT;
            break;
        case -2:
            error_code = SOCKET_ERROR_BAD_ALLOC;
            break;
        case -5:
            error_code = SOCKET_ERROR_BAD_FAMILY;
            break;
        default:
            error_code = SOCKET_ERROR_UNKNOWN;
            break;
    }

    return error_code;
}

void periodic_task(void)
{
    /* this function will be called periodically when used, empty at the moment */
}
/* socket_api function, see socket_api.h for details */
socket_api_handler_t ns_sal_socket_periodic_task(
    const struct socket *socket)
{
    FUNC_ENTRY_TRACE("ns_sal_socket_periodic_task()");
    if (SOCKET_STREAM == socket->family) {
        return periodic_task;
    }
    return NULL;
}

/* socket_api function, see socket_api.h for details */
uint32_t ns_sal_socket_periodic_interval(const struct socket *socket)
{
    FUNC_ENTRY_TRACE("ns_sal_socket_periodic_interval()");
    if (SOCKET_STREAM == socket->family) {
        return 0xffffffff;
    }
    return 0;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_resolve(struct socket *socket,
                                     const char *address)
{
    if (NULL == socket || NULL == socket->impl || NULL == address) {
        return SOCKET_ERROR_NULL_PTR;
    }
    /* TODO: Implement DNS resolving */
    ns_sal_callback_name_resolved(socket, address);
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_str2addr(const struct socket *sock,
                               struct socket_addr *addr, const char *address)
{
    socket_error_t err = SOCKET_ERROR_NONE;
    FUNC_ENTRY_TRACE("ns_sal_str2addr() %s", address);
    if (NULL == sock || NULL == addr || NULL == address) {
        err = SOCKET_ERROR_NULL_PTR;
    } else if (SOCKET_STACK_NANOSTACK_IPV6 != sock->stack) {
        err = SOCKET_ERROR_BAD_STACK;
    } else {
        stoip6(address, strlen(address), addr->ipv6be);
    }

    return err;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_bind(struct socket *socket,
                                  const struct socket_addr *address, const uint16_t port)
{
    ns_address_t ns_address;

    if (NULL == socket || NULL == socket->impl || NULL == address) {
        return SOCKET_ERROR_NULL_PTR;
    }

    convert_mbed_addr_to_ns(&ns_address, address, port);

    if (0 == ns_wrapper_socket_bind(socket->impl, &ns_address)) {
        return SOCKET_ERROR_NONE;
    }

    return SOCKET_ERROR_UNKNOWN;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_start_listen(struct socket *socket,
                                   const uint32_t backlog)
{
    (void) socket;
    (void) backlog;
    tr_error("ns_sal_start_listen() not implemented");
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_stop_listen(struct socket *socket)
{
    (void) socket;
    tr_error("ns_sal_stop_listen() not implemented");
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_accept(struct socket *socket,
                                    socket_api_handler_t handler)
{
    (void) socket;
    (void) handler;
    tr_error("ns_sal_socket_accept() not implemented");
    return SOCKET_ERROR_UNIMPLEMENTED;
}

socket_error_t ns_sal_socket_reject(struct socket *socket)
{
    (void) socket;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_send(struct socket *socket, const void *buf,
                                  const size_t len)
{
    socket_error_t err = SOCKET_ERROR_UNKNOWN;

    if (NULL == socket || NULL == socket->impl) {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (SOCKET_DGRAM == socket->family) {
        // UDP sockets can't be connected in mesh.
        tr_error("send() not supported with SOCKET_DGRAM!");
        err = SOCKET_ERROR_BAD_FAMILY;
    } else if (SOCKET_STREAM == socket->family) {
        int8_t status = ns_wrapper_socket_send(socket->impl, (uint8_t *) buf,
                                               len);
        switch (status) {
            case 0:
                err = SOCKET_ERROR_NONE;
                break;
            case -1:
            case -6:
                err = SOCKET_ERROR_BAD_ARGUMENT;
                break;
            case -2:
                err = SOCKET_ERROR_BAD_ALLOC;
                break;
            case -3:
                err = SOCKET_ERROR_NO_CONNECTION;
                break;
            default:
                /* -4, 5 */
                err = SOCKET_ERROR_UNKNOWN;
                break;
        }
    }
    return err;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_send_to(struct socket *socket, const void *buf,
                                     const size_t len, const struct socket_addr *addr, const uint16_t port)
{
    socket_error_t error_status = SOCKET_ERROR_NONE;
    int8_t send_to_status;

    FUNC_ENTRY_TRACE("ns_sal_socket_send_to()");
    if (NULL == socket || NULL == socket->impl || NULL == buf || NULL == addr) {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (len <= 0) {
        return SOCKET_ERROR_SIZE;
    }

    switch (socket->family) {
        case SOCKET_DGRAM: {
            ns_address_t ns_address;
            convert_mbed_addr_to_ns(&ns_address, addr, port);
            send_to_status = ns_wrapper_socket_send_to(socket->impl,
                             &ns_address, (uint8_t *) buf, len);
            /*
             * \return 0 on success.
             * \return -1 invalid socket id.
             * \return -2 Socket memory allocation fail.
             * \return -3 TCP state not established.
             * \return -4 Socket tx process busy.
             * \return -5 TLS authentication not ready.
             * \return -6 Packet too short.
             * */

            if (0 != send_to_status) {
                tr_error("ns_sal_socket_send_to: error=%d", send_to_status);
                error_status = SOCKET_ERROR_UNKNOWN;
            }
            break;
        }
        case SOCKET_STREAM:
            error_status = SOCKET_ERROR_BAD_FAMILY;
            break;
    }

    return error_status;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_recv(struct socket *socket, void *buf,
                                  size_t *len)
{
    FUNC_ENTRY_TRACE("ns_sal_socket_recv() len=%d", *len);

    socket_error_t err;

    if (SOCKET_DGRAM == socket->family) {
        tr_error("recv() not supported with SOCKET_DGRAM!");
        return SOCKET_ERROR_BAD_FAMILY;
    }

    err = ns_sal_recv_validate(socket, buf, len);
    if (err != SOCKET_ERROR_NONE) {
        return err;
    }

    ns_sal_copy_stream(socket, buf, len);

    //tr_debug("received %d bytes", *len);

    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
socket_error_t ns_sal_socket_recv_from(struct socket *socket, void *buf,
                                       size_t *len, struct socket_addr *addr, uint16_t *port)
{
    /* socket and socket->impl will be validated in ns_sal_recv_validate */
    if (NULL == addr || NULL == port) {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (SOCKET_STREAM == socket->family) {
        tr_error("recv_from() not supported with SOCKET_STREAM!");
        return SOCKET_ERROR_BAD_FAMILY;
    }

    socket_error_t err = ns_sal_recv_validate(socket, buf, len);
    if (err != SOCKET_ERROR_NONE) {
        return err;
    }

    ns_sal_copy_datagrams(socket, buf, len, addr, port);

    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
uint8_t ns_sal_socket_is_connected(const struct socket *socket)
{
    if (SOCKET_DGRAM == socket->family) {
        // UDP sockets can't be connected in NanoStack */
        return 0;
    } else if (SOCKET_STREAM == socket->family) {
        // TODO, implement for TCP sockets
        return 0;
    }

    return 0;
}

/* socket_api function, see socket_api.h for details */
uint8_t ns_sal_socket_is_bound(const struct socket *socket)
{
    // TODO: Not implemented
    (void) socket;
    return 0;
}

socket_error_t ns_sal_socket_get_local_addr(const struct socket *socket, struct socket_addr *addr)
{
    if (socket == NULL || addr == NULL) {
        return SOCKET_ERROR_NULL_PTR;
    }

    return SOCKET_ERROR_UNIMPLEMENTED;
}


socket_error_t ns_sal_socket_get_remote_addr(const struct socket *socket, struct socket_addr *addr)
{
    if (socket == NULL || addr == NULL) {
        return SOCKET_ERROR_NULL_PTR;
    }

    return SOCKET_ERROR_UNIMPLEMENTED;
}

socket_error_t ns_sal_socket_get_local_port(const struct socket *socket, uint16_t *port)
{
    if (socket == NULL || port == NULL) {
        return SOCKET_ERROR_NULL_PTR;
    }
    return SOCKET_ERROR_UNIMPLEMENTED;
}

socket_error_t ns_sal_socket_get_remote_port(const struct socket *socket, uint16_t *port)
{
    if (socket == NULL || port == NULL) {
        return SOCKET_ERROR_NULL_PTR;
    }
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/*
 * Socket API function pointer table.
 */

// Version of socket API, need to match version of SAL.
#define SOCKET_ABSTRACTION_LAYER_VERSION 1
const struct socket_api nanostack_socket_api = {
    .stack = SOCKET_STACK_NANOSTACK_IPV6,
    .version = SOCKET_ABSTRACTION_LAYER_VERSION,
    .init = ns_sal_init,
    .create = ns_sal_socket_create,
    .destroy = ns_sal_socket_destroy,
    .close = ns_sal_socket_close,
    .periodic_task = ns_sal_socket_periodic_task,
    .periodic_interval = ns_sal_socket_periodic_interval,
    .resolve = ns_sal_socket_resolve,
    .connect = ns_sal_socket_connect,
    .str2addr = ns_sal_str2addr,
    .bind = ns_sal_socket_bind,
    .start_listen = ns_sal_start_listen,
    .stop_listen = ns_sal_stop_listen,
    .accept = ns_sal_socket_accept,
    .reject = ns_sal_socket_reject,
    .send = ns_sal_socket_send,
    .send_to = ns_sal_socket_send_to,
    .recv = ns_sal_socket_recv,
    .recv_from = ns_sal_socket_recv_from,
    .is_connected = ns_sal_socket_is_connected,
    .is_bound = ns_sal_socket_is_bound,
    .get_local_addr = ns_sal_socket_get_local_addr,
    .get_remote_addr = ns_sal_socket_get_remote_addr,
    .get_local_port = ns_sal_socket_get_local_port,
    .get_remote_port = ns_sal_socket_get_remote_port
};
