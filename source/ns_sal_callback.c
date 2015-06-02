/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * NanoStack Socket Abstraction Layer (SAL) callback functions.
 * These callbacks will be called from NanoStack when some event occurs.
 */
#include <string.h> // strlen
#include "ns_address.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include "mbed-6lowpan-adaptor/ns_sal_callback.h"
#include "ip6string.h"  //nanostack stoip6
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "ns_sal_cb"

/*
 * \brief Send callback to mbed client
 * \param socket socket to be used
 * \param e event to be send
 */
void send_socket_callback(struct socket *socket, socket_event_t *e)
{
    tr_debug("send_socket_callback() event=%d", e->event);
    socket->event = e;
    ((socket_api_handler_t) (socket->handler))();
    socket->event = NULL;
}

void ns_sal_callback_name_resolved(void* context, const char *address)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_DNS;
    e.i.d.addr.type = SOCKET_STACK_NANOSTACK_IPV6;
    e.i.d.domain = address;
    stoip6(address, strlen(address), e.i.d.addr.storage);
    send_socket_callback(socket, &e);
}

void ns_sal_callback_data_received(void* context, data_buff_t *data_buf)
{
    socket_event_t e;
    data_buff_t *buf_tmp;
    struct socket *socket = (struct socket*) context;

    /*
     * data_buf can be NULL when we want to inform client that more data should
     *  be read from the buffer
     */
    if (NULL != data_buf)
    {
        if (NULL == socket->rxBufChain)
        {
            socket->rxBufChain = data_buf;
        }
        else
        {
            buf_tmp = (data_buff_t*) socket->rxBufChain;
            while (NULL != buf_tmp->next)
            {
                buf_tmp = buf_tmp->next;
            }
            buf_tmp->next = data_buf;
        }
    }

    e.event = SOCKET_EVENT_RX_DONE;
    e.sock = socket;
    e.i.e = SOCKET_ERROR_NONE;

    send_socket_callback(socket, &e);
}

/*
 * Callback from NanoStack socket, data sent.
 */
void ns_sal_callback_tx_done(void* context, uint16_t length)
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
void ns_sal_callback_tx_error(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_TX_ERROR;
    e.sock = socket;
    e.i.t.sentbytes = 0;
    send_socket_callback(socket, &e);
}

void ns_sal_callback_connect(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_CONNECT;
    socket->status |= SOCKET_STATUS_CONNECTED;
    e.sock = socket;
    send_socket_callback(socket, &e);
}

void ns_sal_callback_disconnect(void* context)
{
    socket_event_t e;
    struct socket *socket = (struct socket*) context;
    e.event = SOCKET_EVENT_DISCONNECT;
    socket->status &= ~SOCKET_STATUS_CONNECTED;
    e.sock = socket;
    send_socket_callback(socket, &e);
}

