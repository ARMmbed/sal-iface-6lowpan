/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
/*
 * Mesh networking interface.
 */

#include "ns_address.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include "mbed-6lowpan-adaptor/ns_sal.h"
#include "mbed-6lowpan-adaptor/ns_wrapper.h"
#include "mbed-6lowpan-adaptor/mesh_interface.h"
// For tracing we need to define flag, have include and define group
#define HAVE_DEBUG 1
#include "ns_trace.h"
#define TRACE_GROUP  "mesh_interface"

/*
 * \brief NanoStack initialization method. Called by application.
 */
socket_error_t mesh_interface_init(void)
{
    return ns_sal_init_stack();
}

/*
 * \brief Connect NanoStack to mesh network
 * \param callback_func, function that is called when network is established.
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when connection can't be made.
 */
socket_error_t mesh_interface_connect(network_ready_cb_t callback_func)
{
    int8_t retval;
    retval = ns_wrapper_socket_network_connect(callback_func);
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
socket_error_t mesh_interface_disconnect(void)
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
socket_error_t mesh_interface_get_ip_address(char *address, int8_t len)
{
    if (0 == ns_wrapper_get_ip_address(address, len))
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
socket_error_t mesh_interface_get_router_ip_address(char *address, int8_t len)
{
    if (0 == ns_wrapper_get_router_ip_address(address, len))
    {
        return SOCKET_ERROR_NONE;
    }

    return SOCKET_ERROR_UNKNOWN;
}

/*
 * \brief Handle NanoStack events.
 */
void mesh_interface_run(void)
{
    ns_wrapper_run();
}
