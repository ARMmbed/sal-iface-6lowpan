/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * NanoStack adaptation to mbed socket API.
 */

#include <string.h> // memcpy
#include <stdio.h>

#include <mbed-net-socket-abstract/socket_api.h>
#include "ns_address.h"
#include "mbed-6lowpan-adaptor/nanostack_socket_impl.h"
#include "net_interface.h"
#include "ip6string.h"  //stoip6
#include "mbed-6lowpan-adaptor/nanostack_callback.h"
#include "common_functions.h"
#include "nsdynmemLIB.h"
#include "mbed-6lowpan-adaptor/nanostack_init.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "ns_sock"

#define MALLOC  ns_dyn_mem_alloc
#define FREE    ns_dyn_mem_free

// Forward declaration of this socket_api
const struct socket_api nanostack_socket_api;

void send_socket_callback(struct socket *socket, socket_event_t *e);

/*** PUBLIC METHODS ***/
/*
 * \brief NanoStack initialization method. Called by application.
 */
socket_error_t nanostack_init(void)
{
    int8_t retval = nanostack_socket_init_impl();
    if (0 > retval)
    {
        return SOCKET_ERROR_UNKNOWN;
    }

    return socket_register_stack(&nanostack_socket_api);
}

/*
 * \brief Connect NanoStack to mesh network
 * \param callback_func, function that is called when network is established.
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when connection can't be made.
 */
socket_error_t nanostack_connect(network_ready_cb_t callback_func)
{
    int8_t retval;
    retval = nanostack_socket_network_connect_impl(callback_func);
    if (0 > retval)
    {
        return SOCKET_ERROR_UNKNOWN;
    }
    else
    {
        return SOCKET_ERROR_NONE;
    }
}

/*
 * \brief Disconnect NanoStack from mesh network
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when disconnect fails
 */
socket_error_t nanostack_disconnect(void)
{
    tr_debug("nanostack_disconnect()");
    return SOCKET_ERROR_NONE;
}

/*
 * \brief Get node IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t nanostack_get_ip_address(char *address, int8_t len)
{
    if (0 == nanostack_get_ip_address_impl(address, len))
    {
        return SOCKET_ERROR_NONE;
    }

    return SOCKET_ERROR_UNKNOWN;
}

/*
 * \brief Get router IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t nanostack_get_router_ip_address(char *address, int8_t len)
{
    if (0 == nanostack_get_router_ip_address_impl(address, len))
    {
        return SOCKET_ERROR_NONE;
    }

    return SOCKET_ERROR_UNKNOWN;
}

/*
 * \brief Handle NanoStack events.
 */
void nanostack_run(void)
{
    nanostack_run_impl();
}

/*** PRIVATE METHODS ***/

/*
 * translate mbed address to NanoStack address structure.
 */
void transform_addr_to_ns(ns_address_t *ns_addr,
        const struct socket_addr *s_addr, uint16_t port)
{
    ns_addr->type = ADDRESS_IPV6;
    ns_addr->identifier = port;
    memcpy(ns_addr->address, s_addr->storage, 16);
}

/*
 * translate NanoStack address to mbed address.
 */
void transform_addr_to_mbed(struct socket_addr *s_addr, ns_address_t *ns_addr,
        uint16_t *port)
{
    *port = ns_addr->identifier;
    if (ADDRESS_IPV4 == ns_addr->type)
    {
        // TODO: Fix address type when it is available in the API
        s_addr->type = SOCKET_STACK_NANOSTACK_IPV6;
    }
    else
    {
        s_addr->type = SOCKET_STACK_NANOSTACK_IPV6;
    }
    memcpy(s_addr->storage, ns_addr->address, 16);
}

socket_error_t recv_validate(struct socket *socket, void * buf, size_t *len)
{
    if (socket == NULL || len == NULL || buf == NULL || socket->impl == NULL)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (*len == 0)
    {
        return SOCKET_ERROR_SIZE;
    }

    return SOCKET_ERROR_NONE;
}

/*** SOCKET API METHODS ***/

/* socket_api function, see socket_api.h for details */
static socket_error_t init()
{
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t nanostack_socket_create(struct socket *sock,
        const socket_address_family_t af, const socket_proto_family_t pf,
        socket_api_handler_t const handler)
{
    sock_data_s *sock_data_ptr;

    tr_debug("nanostack_socket_create() af=%d pf=%d", af, pf);

    if (NULL == sock || NULL == handler)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (SOCKET_AF_INET6 != af)
    {
        // Only ipv6 is supported by this stack
        return SOCKET_ERROR_BAD_FAMILY;
    }

    sock->stack = SOCKET_STACK_NANOSTACK_IPV6;
    switch (pf)
    {
    case SOCKET_DGRAM:
        sock_data_ptr = nanostack_socket_open_impl(NANOSTACK_SOCKET_UDP, 0,
                (void*) sock);
    break;
    case SOCKET_STREAM:
        sock_data_ptr = nanostack_socket_open_impl(NANOSTACK_SOCKET_TCP, 0,
                (void*) sock);
    break;
    default:
        return SOCKET_ERROR_BAD_FAMILY;
    }

    if (-1 == sock_data_ptr->socket_id)
    {
        nanostack_release_socket_data_impl(sock_data_ptr);
        return SOCKET_ERROR_UNKNOWN;
    }
    sock->impl = sock_data_ptr;
    sock->family = pf;
    sock->handler = (void*) handler;
    sock->status = SOCKET_STATUS_IDLE;
    sock->rxBufChain = NULL;
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t nanostack_socket_destroy(struct socket *sock)
{
    socket_error_t err = SOCKET_ERROR_NONE;
    tr_debug("nanostack_socket_destroy()");
    if (NULL == sock)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (NULL != sock->impl)
    {
        int8_t status = nanostack_socket_free_impl(sock->impl);
        sock->impl = NULL;
        if (0 != status)
        {
            err = SOCKET_ERROR_UNKNOWN;
        }
    }

    return err;
}

/* socket_api function, see socket_api.h for details */
static socket_error_t nanostack_socket_close(struct socket *sock)
{
    socket_error_t error = SOCKET_ERROR_UNKNOWN;
    int8_t return_value;
    if (NULL == sock)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    return_value = nanostack_socket_close_impl(sock->impl);

    switch (return_value)
    {
    case 0:
        error = SOCKET_ERROR_NONE;
    break;
    case -1:
        // -1 if a given socket ID is not found, if a socket type is wrong or tcp_close() returns a failure.
        error = SOCKET_ERROR_BAD_FAMILY;
    break;
    case -2:
        // -2 if no active tcp session was found.
        // pass through
    case -3:
        // -3 if referred socket ID port is declared as a zero.
        // pass through
    default:
        error = SOCKET_ERROR_UNKNOWN;
    }
    return error;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_connect(struct socket *sock,
        const struct socket_addr *address, const uint16_t port)
{
    socket_error_t error_code;
    if (NULL == sock || NULL == address)
    {
        return SOCKET_ERROR_NULL_PTR;

    }
    else if (SOCKET_STACK_NANOSTACK_IPV6 != address->type)
    {
        return SOCKET_ERROR_BAD_ADDRESS;
    }

    ns_address_t ns_address;
    transform_addr_to_ns(&ns_address, address, port);
    int8_t retval = nanostack_socket_connect_impl(sock->impl, &ns_address);
    switch (retval)
    {
    case 0:
        error_code = SOCKET_ERROR_NONE;
    break;
        // TODO: Find correct mappings for these errors
    case -1:
        case -2:
        case -3:
        case -4:
        default:
        error_code = SOCKET_ERROR_UNKNOWN;
    break;
    }

    return error_code;
}

void periodic_task(void)
{
    /* this will be called periodically when used, empty at the moment */
}
/* socket_api function, see socket_api.h for details */
socket_api_handler_t nanostack_socket_periodic_task(
        const struct socket * socket)
{
    tr_debug("nanostack_socket_periodic_task()");
    if (SOCKET_STREAM == socket->family)
    {
        return periodic_task;
    }
    return NULL;
}

/* socket_api function, see socket_api.h for details */
uint32_t nanostack_socket_periodic_interval(const struct socket * socket)
{
    tr_debug("nanostack_socket_periodic_interval()");
    if (SOCKET_STREAM == socket->family)
    {
        return 0xffffffff;
    }
    return 0;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_resolve(struct socket *socket,
        const char *address)
{
    socket_event_t e;
    e.event = SOCKET_EVENT_DNS;
    e.i.d.addr.type = SOCKET_STACK_NANOSTACK_IPV6;
    e.i.d.domain = address;
    stoip6(address, strlen(address), e.i.d.addr.storage);
    send_socket_callback(socket, &e);
    return SOCKET_ERROR_NONE;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_str2addr(const struct socket *sock,
        struct socket_addr *addr, const char *address)
{
    socket_error_t err = SOCKET_ERROR_NONE;
    if (NULL == addr || NULL == address)
    {
        err = SOCKET_ERROR_NULL_PTR;
    }
    else if (SOCKET_STACK_NANOSTACK_IPV6 != sock->stack)
    {
        err = SOCKET_ERROR_BAD_STACK;
        addr->type = SOCKET_STACK_UNINIT;
    }
    else
    {
        stoip6(address, strlen(address), addr->storage);
        addr->type = sock->stack;
    }

    return err;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_bind(struct socket *socket,
        const struct socket_addr *address, const uint16_t port)
{
    /* Note, binding is not supported for UDP sockets */
    (void) socket;
    (void) address;
    (void) port;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_start_listen(struct socket *socket,
        const uint32_t backlog)
{
    (void) socket;
    (void) backlog;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_stop_listen(struct socket *socket)
{
    (void) socket;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_accept(struct socket *socket,
        socket_api_handler_t handler)
{
    (void) socket;
    (void) handler;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_send(struct socket *socket, const void * buf,
        const size_t len)
{
    socket_error_t err = SOCKET_ERROR_UNIMPLEMENTED;
    if (SOCKET_DGRAM == socket->family)
    {
        // TODO
    }
    else if (SOCKET_STREAM == socket->family)
    {
        int8_t status = nanostack_socket_send_impl(socket->impl, (uint8_t*) buf,
                len);
        switch (status)
        {
        case 0:
            err = SOCKET_ERROR_NONE;
        break;
        case -2:
            err = SOCKET_ERROR_BAD_ALLOC;
        break;
        case -3:
            err = SOCKET_ERROR_NO_CONNECTION;
        break;
        default:
            /* -1, -4, 5, 6 */
            err = SOCKET_ERROR_UNKNOWN;
        break;
        }
    }
    return err;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_send_to(struct socket *socket, const void * buf,
        const size_t len, const struct socket_addr *addr, const uint16_t port)
{
    socket_error_t error_status = SOCKET_ERROR_NONE;
    int8_t send_to_status;

    tr_debug("nanostack_socket_send_to()");
    if (NULL == socket || NULL == buf || 0 >= len)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    switch (socket->family)
    {
    case SOCKET_DGRAM:
    {
        ns_address_t ns_address;
        transform_addr_to_ns(&ns_address, addr, port);
        send_to_status = nanostack_socket_send_to_impl(socket->impl,
                &ns_address, (uint8_t*) buf, len);
        /*
         * \return 0 on success.
         * \return -1 invalid socket id.
         * \return -2 Socket memory allocation fail.
         * \return -3 TCP state not established.
         * \return -4 Socket tx process busy.
         * \return -5 TLS authentication not ready.
         * \return -6 Packet too short.
         * */

        if (0 != send_to_status)
        {
            tr_error("nanostack_socket_send_to: error=%d", send_to_status);
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
socket_error_t nanostack_socket_recv(struct socket *socket, void * buf,
        size_t *len)
{
    (void) socket;
    (void)buf;
    (void) len;
    return SOCKET_ERROR_UNIMPLEMENTED;
}

/* socket_api function, see socket_api.h for details */
socket_error_t nanostack_socket_recv_from(struct socket *socket, void *buf,
        size_t *len, struct socket_addr *addr, uint16_t *port)
{
    socket_error_t err = recv_validate(socket, buf, len);
    if (err != SOCKET_ERROR_NONE)
    {
        return err;
    }

    if (addr == NULL || port == NULL)
    {
        return SOCKET_ERROR_NULL_PTR;
    }

    if (*len < 1)
    {
        tr_error("Receive buffer too small %d", *len);
        return SOCKET_ERROR_SIZE;
    }

    ns_address_t ns_address;
    transform_addr_to_ns(&ns_address, addr, *port);

    int8_t read_len = nanostack_socket_recv_from_imp(socket->impl, &ns_address,
            (uint8_t*) buf, *len);

    if (read_len > 0)
    {
        transform_addr_to_mbed(addr, &ns_address, port);
        *len = read_len;
        return SOCKET_ERROR_NONE;
    }

    return SOCKET_ERROR_UNKNOWN;
}

/* socket_api function, see socket_api.h for details */
uint8_t nanostack_socket_is_connected(const struct socket *socket)
{
    // TODO: Not implemented
    (void) socket;
    return 0;
}

/* socket_api function, see socket_api.h for details */
uint8_t nanostack_socket_is_bound(const struct socket *socket)
{
    // TODO: Not implemented
    (void) socket;
    return 0;
}

/* socket_api function, see socket_api.h for details */
uint8_t nanostack_socket_tx_is_busy(const struct socket *socket)
{
    // TODO: Not implemented
    (void) socket;
    return 0;
}

/* socket_api function, see socket_api.h for details */
uint8_t nanostack_socket_rx_is_busy(const struct socket *socket)
{
    // TODO: Not implemented
    (void) socket;
    return 0;
}

/*
 * Socket handling functions
 */
const struct socket_api nanostack_socket_api =
        {
                .stack = SOCKET_STACK_NANOSTACK_IPV6,
                .init = init,
                .create = nanostack_socket_create,
                .destroy = nanostack_socket_destroy,
                .close = nanostack_socket_close,
                .periodic_task = nanostack_socket_periodic_task,
                .periodic_interval = nanostack_socket_periodic_interval,
                .resolve = nanostack_socket_resolve,
                .connect = nanostack_socket_connect,
                .str2addr = nanostack_str2addr,
                .bind = nanostack_socket_bind,
                .start_listen = nanostack_start_listen,
                .stop_listen = nanostack_stop_listen,
                .accept = nanostack_socket_accept,
                .send = nanostack_socket_send,
                .send_to = nanostack_socket_send_to,
                .recv = nanostack_socket_recv,
                .recv_from = nanostack_socket_recv_from,
                .is_connected = nanostack_socket_is_connected,
                .is_bound = nanostack_socket_is_bound,
                .tx_busy = nanostack_socket_tx_is_busy,
                .rx_busy = nanostack_socket_rx_is_busy
        };

/*
 * Callback from NanoStack socket, data received. Inform client that data should be read.
 */
void nanostack_data_received_callback(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_RX_DONE;
    e.sock = socket;
    e.i.e = SOCKET_ERROR_NONE;

    send_socket_callback(socket, &e);
}

/*
 * Callback from NanoStack socket, data sent.
 */
void nanostack_tx_done_callback(void* context, uint16_t length)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_TX_DONE;
    e.sock = socket;
    e.i.t.sentbytes = length;
    send_socket_callback(socket, &e);
}

/*
 * Callback from NanoStack socket, data sending error.
 */
void nanostack_tx_error_callback(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_TX_ERROR;
    e.sock = socket;
    e.i.t.sentbytes = 0;
    send_socket_callback(socket, &e);
}

void nanostack_connect_callback(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_CONNECT;
    socket->status |= SOCKET_STATUS_CONNECTED;
    e.sock = socket;
    send_socket_callback(socket, &e);
}

/*
 * \brief Send callback to mbed socket
 */
void send_socket_callback(struct socket *socket, socket_event_t *e)
{

    tr_debug("send_socket_callback() event=%d", e->event);
    socket->event = e;
    ((socket_api_handler_t) (socket->handler))();
    socket->event = NULL;
}

