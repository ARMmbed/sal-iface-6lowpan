/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * NanoStack adaptation to mbed socket AP, implementation.
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
#include "mbed-6lowpan-adaptor/nanostack_socket_impl.h"
#include "mbed-6lowpan-adaptor/node_tasklet.h"
#include "net_interface.h"
#include "socket_api.h"	// nanostack(6lowpan) socket api
#include "mbed-6lowpan-adaptor/nanostack_callback.h"
#include "atmel-rf-driver/driverRFPhy.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "ns_sock_impl"

#define SOCKETS_MAX 16  // TODO, fix double definition
#define MALLOC  ns_dyn_mem_alloc
#define FREE    ns_dyn_mem_free

/* Heap for NanoStack */
#define APP_DEFINED_HEAP_SIZE 32500
static uint8_t app_stack_heap[APP_DEFINED_HEAP_SIZE +1];

// table for socket contexts
typedef struct _socket_context_map_t {
    void* context;
} socket_context_map_t;

static socket_context_map_t socket_context_tbl[SOCKETS_MAX] = {{0}};

/* Prototypes */
static void heap_error_handler(heap_fail_t event);
void socket_callback(void *cb);

/* External prototypes */
extern void event_dispatch_cycle(void);

static void heap_error_handler(heap_fail_t event)
{
    tr_error("Error, heap_error_handler()");
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
 * \brief NanoStack initialization routine
 * \return >= 0 on success
 * \return negative number on error
 */
int8_t nanostack_socket_init_impl(void)
{
    int8_t rf_phy_device_register_id;
    char if_desciption[] = "6LoWPAN_NODE";
    ns_dyn_mem_init(app_stack_heap, APP_DEFINED_HEAP_SIZE, heap_error_handler, 0);
    randLIB_seed_random();
    platform_timer_enable();
    eventOS_scheduler_init();
    trace_init(); // trace system needs to be initialized right after eventOS_scheduler_init
    net_init_core();
    rf_phy_device_register_id = rf_device_register();
    network_interface_id = arm_nwk_interface_init(NET_INTERFACE_RF_6LOWPAN, rf_phy_device_register_id, if_desciption);
    return network_interface_id;
}

int8_t nanostack_get_ip_address_impl(char *address, int8_t len)
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

int8_t nanostack_get_router_ip_address_impl(char *address, int8_t len)
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

int8_t nanostack_socket_network_connect_impl(func_cb_t callback)
{
    int8_t error_status;
    error_status = eventOS_event_handler_create(&tasklet_main, ARM_LIB_TASKLET_INIT_EVENT);
    if (0 > error_status) {
        return error_status;
    }
    node_tasklet_register_network_ready_cb(callback);
    return error_status;
}

sock_data_s* create_socket_data(void)
{
    sock_data_s * data = (sock_data_s *) MALLOC(
            sizeof(sock_data_s));
    data->socket_id = -1; // -1 indicates failure
    data->security_session_id = 0;
    return data;
}

void nanostack_release_socket_data_impl(sock_data_s *sock_data_ptr)
{
    FREE(sock_data_ptr);
}

int8_t nanostack_socket_free_impl(sock_data_s *sock_data_ptr)
{
    tr_debug("nanostack_socket_free_impl() sock=%d", sock_data_ptr->socket_id);
    int8_t retval = socket_free(sock_data_ptr->socket_id);
    nanostack_release_socket_data_impl(sock_data_ptr);
    return retval;
}

sock_data_s* nanostack_socket_open_impl(int8_t socket_type, int8_t identifier, void *context)
{
    int8_t protocol = SOCKET_TCP;
    if (NANOSTACK_SOCKET_UDP == socket_type)
    {
        protocol = SOCKET_UDP;
    }

    sock_data_s *sock_data_ptr = create_socket_data();
    sock_data_ptr->socket_id = socket_open(protocol, identifier, socket_callback);
    // save context to table so that callbacks can be made to right socket
    socket_context_tbl[sock_data_ptr->socket_id].context = context;

    return sock_data_ptr;
}

int8_t nanostack_socket_close_impl(sock_data_s *sock_data_ptr)
{
    tr_debug("nanostack_socket_close_impl() sock=%d", sock_data_ptr->socket_id);
    int8_t error = socket_close(sock_data_ptr->socket_id, NULL,
            sock_data_ptr->security_session_id);
    return error;
}

int8_t nanostack_socket_connect_impl(sock_data_s *sock_data_ptr, ns_address_t *address)
{
    return socket_connect(sock_data_ptr->socket_id, address, 0);
}

int8_t nanostack_socket_send_impl(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length)
{
    tr_debug("nanostack_socket_send_impl: sock_id=%d, length=%d", sock_data_ptr->socket_id, length);
    return socket_send(sock_data_ptr->socket_id, buffer, length);
}

int8_t nanostack_socket_send_to_impl(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length)
{
    tr_debug("nanostack_socket_send_to_impl: sock_id=%d, length=%d, port=%d", sock_data_ptr->socket_id, length, addr->identifier);
    return socket_sendto(sock_data_ptr->socket_id, addr, buffer, length);
}

int8_t nanostack_socket_recv_from_imp(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length)
{
    tr_debug("nanostack_socket_recv_from_imp: sock_id=%d, buf len=%d, port=%d", sock_data_ptr->socket_id, length, addr->identifier);
    return socket_read(sock_data_ptr->socket_id, addr, buffer, length);
}

void nanostack_run_impl(void)
{
    event_dispatch_cycle();
}



/*
 * Socket callback, will be called automatically by the socket.
 */
void socket_callback(void *cb)
{
    socket_callback_t *sock_cb = (socket_callback_t *) cb;

    tr_debug("socket_callback() sock=%d, event=%d, interface=%d, data len=%d",
            sock_cb->socket_id, sock_cb->event_type, sock_cb->interface_id, sock_cb->d_len);

    switch (sock_cb->event_type)
    {
    case SOCKET_DATA:
        tr_debug("SOCKET_DATA, %d bytes", sock_cb->d_len);
        nanostack_data_received_callback(socket_context_tbl[sock_cb->socket_id].context);
        break;
    case SOCKET_BIND_DONE:
        tr_debug("SOCKET_BIND_DONE");
        nanostack_connect_callback(socket_context_tbl[sock_cb->socket_id].context);
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
    break;
    case SOCKET_CONNECT_FAIL_CLOSED:
        tr_debug("SOCKET_CONNECT_FAIL_CLOSED");
    break;
    case SOCKET_NO_ROUTE:
        tr_debug("SOCKET_NO_ROUTE");
    break;
    case SOCKET_TX_DONE:
        tr_debug("SOCKET_TX_DONE, %d bytes sent", sock_cb->d_len);
        nanostack_tx_done_callback(socket_context_tbl[sock_cb->socket_id].context, sock_cb->d_len);
    break;

    default:
        // SOCKET_NO_RAM, error case for SOCKET_TX_DONE
        nanostack_tx_error_callback(socket_context_tbl[sock_cb->socket_id].context);
        break;
    }
}

