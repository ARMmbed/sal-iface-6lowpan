/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "eventOS_event.h"
#include "eventOS_scheduler.h"
#include "nsdynmemLIB.h"
#include "randLIB.h"
#include "ip6string.h"  //ip6tos
#include "platform/arm_hal_timer.h"
#include "ns_address.h"
#include "mbed-6lowpan-adaptor/node_tasklet.h"
#include "mbed-6lowpan-adaptor/ns_sal_callback.h"
#include "net_interface.h"
#include "socket_api.h"	// nanostack(6lowpan) socket api
#include "atmel-rf-driver/driverRFPhy.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "ns_wrapper"
#include "mbed-6lowpan-adaptor/ns_wrapper.h"

#define NS_WRAPPER_SOCKETS_MAX  16  //same as NanoStack SOCKET_MAX
#define MALLOC  ns_dyn_mem_alloc
#define FREE    ns_dyn_mem_free

/* Heap for NanoStack */
#define APP_DEFINED_HEAP_SIZE 32500
static uint8_t app_stack_heap[APP_DEFINED_HEAP_SIZE +1];

// table for socket contexts
typedef struct _socket_context_map_t {
    void* context;
} socket_context_map_t;

static socket_context_map_t socket_context_tbl[NS_WRAPPER_SOCKETS_MAX] = {{0}};

/* Prototypes */

/* External prototypes */
extern void event_dispatch_cycle(void);

/**** Private functions ****/
/*
 * Memory error handler
 */
static void ns_wrapper_heap_error_handler(heap_fail_t event)
{
    tr_error("Error, ns_wrapper_heap_error_handler() %d", event);
    switch (event)
    {
        case NS_DYN_MEM_NULL_FREE:
        case NS_DYN_MEM_DOUBLE_FREE:
        case NS_DYN_MEM_ALLOCATE_SIZE_NOT_VALID:
        case NS_DYN_MEM_POINTER_NOT_VALID:
        case NS_DYN_MEM_HEAP_SECTOR_CORRUPTED:
        case NS_DYN_MEM_HEAP_SECTOR_UNITIALIZED:
            break;
        default:
            break;
    }
    while(1);
}

/*
 * Handler for received data
 */
void ns_wrapper_data_received(socket_callback_t *sock_cb) {
    if (sock_cb->d_len > 0)
    {
        data_buff_t *recv_buff = (data_buff_t *) ns_dyn_mem_alloc(
                sizeof(data_buff_t) + sock_cb->d_len);
        if (NULL != recv_buff)
        {
            int16_t length = socket_read(sock_cb->socket_id,
                    &recv_buff->ns_address, recv_buff->payload,
                    sock_cb->d_len);
            recv_buff->length = length;
            recv_buff->next = NULL;

            ns_sal_callback_data_received(socket_context_tbl[sock_cb->socket_id].context, recv_buff);
            // allocated memory will be deallocated when application reads the data or when socket is closed
        }
    }
}

/*
 * Socket callback, called automatically by the NanoStack when event occurs.
 */
void ns_wrapper_socket_callback(void *cb)
{
    socket_callback_t *sock_cb = (socket_callback_t *) cb;

    tr_debug("socket_callback() sock=%d, event=%d, interface=%d, data len=%d",
            sock_cb->socket_id, sock_cb->event_type, sock_cb->interface_id, sock_cb->d_len);

    switch (sock_cb->event_type)
    {
    case SOCKET_DATA:
        tr_debug("SOCKET_DATA, %d bytes", sock_cb->d_len);
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
    case SOCKET_SERVER_CONNECT_TO_CLIENT:
        tr_debug("SOCKET_SERVER_CONNECT_TO_CLIENT");
    break;
    case SOCKET_TX_FAIL:
        tr_debug("SOCKET_TX_FAIL");
    break;
    case SOCKET_CONNECT_CLOSED:
        tr_debug("SOCKET_CONNECT_CLOSED");
        ns_sal_callback_disconnect(socket_context_tbl[sock_cb->socket_id].context);
    break;
    case SOCKET_CONNECT_FAIL_CLOSED:
        tr_debug("SOCKET_CONNECT_FAIL_CLOSED");
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

/**** Public functions ****/
int8_t ns_wrapper_socket_init(void)
{
    int8_t rf_phy_device_register_id;
    char if_desciption[] = "6LoWPAN_NODE";
    ns_dyn_mem_init(app_stack_heap, APP_DEFINED_HEAP_SIZE, ns_wrapper_heap_error_handler, 0);
    randLIB_seed_random();
    platform_timer_enable();
    eventOS_scheduler_init();
    trace_init(); // trace system needs to be initialized right after eventOS_scheduler_init
    net_init_core();
    rf_phy_device_register_id = rf_device_register();
    network_interface_id = arm_nwk_interface_init(NET_INTERFACE_RF_6LOWPAN, rf_phy_device_register_id, if_desciption);
    return network_interface_id;
}

int8_t ns_wrapper_get_ip_address(char *address, int8_t len)
{
    uint8_t binary_ipv6[16];

    if ((len > 40) && (0 == arm_net_address_get(network_interface_id, ADDR_IPV6_GP, binary_ipv6)))
    {
        ip6tos(binary_ipv6, address);
        tr_info("IP address: %s", address);
        return 0;
    } else
    {
        return -1;
    }
}

int8_t ns_wrapper_get_router_ip_address(char *address, int8_t len)
{
    network_layer_address_s nd_address;

    if ((len > 40) && (0 == arm_nwk_nd_address_read(network_interface_id, &nd_address)))
    {
        ip6tos(nd_address.border_router, address);
        tr_info("Router IP address: %s", address);
        return 0;
    } else
    {
        return -1;
    }
}

int8_t ns_wrapper_socket_network_connect(func_cb_t callback)
{
    int8_t error_status;
    error_status = eventOS_event_handler_create(&tasklet_main, ARM_LIB_TASKLET_INIT_EVENT);
    if (0 > error_status) {
        return error_status;
    }
    node_tasklet_register_network_ready_cb(callback);
    return error_status;
}

void ns_wrapper_release_socket_data(sock_data_s *sock_data_ptr)
{
    FREE(sock_data_ptr);
}

int8_t ns_wrapper_socket_free(sock_data_s *sock_data_ptr)
{
    tr_debug("ns_wrapper_socket_free() sock=%d", sock_data_ptr->socket_id);
    int8_t retval = socket_free(sock_data_ptr->socket_id);
    ns_wrapper_release_socket_data(sock_data_ptr);
    return retval;
}

sock_data_s* ns_wrapper_socket_open(int8_t socket_type, int8_t identifier, void *context)
{
    int8_t protocol = SOCKET_TCP;
    if (NANOSTACK_SOCKET_UDP == socket_type)
    {
        protocol = SOCKET_UDP;
    }

    sock_data_s *sock_data_ptr = (sock_data_s *) MALLOC(sizeof(sock_data_s));
    if (NULL != sock_data_ptr)
    {
        sock_data_ptr->socket_id = socket_open(protocol, identifier,
                ns_wrapper_socket_callback);
        if ((sock_data_ptr->socket_id >= 0) &&
                (sock_data_ptr->socket_id < NS_WRAPPER_SOCKETS_MAX))
        {
            sock_data_ptr->security_session_id = 0;
            // save context to table so that callbacks can be made to right socket
            socket_context_tbl[sock_data_ptr->socket_id].context = context;
        }
        else
        {
            /* socket opening failed, free reserved data */
            FREE(sock_data_ptr);
            sock_data_ptr = NULL;
        }
    }

    return sock_data_ptr;
}

int8_t ns_wrapper_socket_bind(sock_data_s *sock_data_ptr, ns_address_t *address)
{
    return socket_bind(sock_data_ptr->socket_id, address);
}

int8_t ns_wrapper_socket_close(sock_data_s *sock_data_ptr)
{
    tr_debug("ns_wrapper_socket_close() sock=%d", sock_data_ptr->socket_id);
    int8_t error = socket_close(sock_data_ptr->socket_id, NULL,
            sock_data_ptr->security_session_id);
    return error;
}

int8_t ns_wrapper_socket_connect(sock_data_s *sock_data_ptr, ns_address_t *address)
{
    tr_debug("ns_wrapper_socket_connect() sock=%d", sock_data_ptr->socket_id);
    return socket_connect(sock_data_ptr->socket_id, address, 0);
}

int8_t ns_wrapper_socket_send(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length)
{
    tr_debug("ns_wrapper_socket_send: sock_id=%d, length=%d", sock_data_ptr->socket_id, length);
    return socket_send(sock_data_ptr->socket_id, buffer, length);
}

int8_t ns_wrapper_socket_send_to(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length)
{
    tr_debug("ns_wrapper_socket_send_to: sock_id=%d, length=%d, port=%d", sock_data_ptr->socket_id, length, addr->identifier);
    return socket_sendto(sock_data_ptr->socket_id, addr, buffer, length);
}

int8_t ns_wrapper_socket_recv_from(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length)
{
    tr_debug("ns_wrapper_socket_recv_from: sock_id=%d, buf len=%d, port=%d", sock_data_ptr->socket_id, length, addr->identifier);
    return socket_read(sock_data_ptr->socket_id, addr, buffer, length);
}

void ns_wrapper_run(void)
{
    event_dispatch_cycle();
}
