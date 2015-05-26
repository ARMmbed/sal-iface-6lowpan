/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#ifndef _MESH_INTERFACE_H_
#define _MESH_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*network_ready_cb_t)(void);

/*
 * \brief Mesh network initialization method. Called by application.
 */
socket_error_t mesh_interface_init(void);

/*
 * \brief Connect node to mesh network
 * \param network_ready_cb is a callback function that is called when network is established.
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when connection can't be made.
 */
socket_error_t mesh_interface_connect(network_ready_cb_t callback_func);

/*
 * \brief Disconnect node from mesh network
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when disconnect fails
 */
socket_error_t mesh_interface_disconnect(void);

/*
 * \brief Get node IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t mesh_interface_get_ip_address(char *address, int8_t len);

/*
 * \brief Get router IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t mesh_interface_get_router_ip_address(char *address, int8_t len);

/*
 * \brief Mesh network event running. Stack is using events in the background and
 * application needs to call this function constantly to keep stack running.
 */
void mesh_interface_run(void);

#ifdef __cplusplus
}
#endif
#endif // _MESH_INTERFACE_H_
