/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright 2015 ARM Holdings PLC
 */
#ifndef MBED_NANOSTACK_INIT_H_
#define MBED_NANOSTACK_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*network_ready_cb_t)(void);

/*
 * \brief NanoStack initialization method. Called by application.
 */
socket_error_t nanostack_init(void);

/*
 * \brief Connect NanoStack to mesh network
 * \param network_ready_cb is a callback function that is called when network is established.
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when connection can't be made.
 */
socket_error_t nanostack_connect(network_ready_cb_t callback_func);

/*
 * \brief Disconnect NanoStack from mesh network
 * \return SOCKET_ERROR_NONE on success.
 * \return SOCKET_ERROR_UNKNOWN when disconnect fails
 */
socket_error_t nanostack_disconnect(void);

/*
 * \brief Get node IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t nanostack_get_ip_address(char *address, int8_t len);

/*
 * \brief Get router IP address. This method can be called when network is established.
 * \param address buffer where address will be read
 * \param length of the buffer, must be at least 40 characters.
 * \return SOCKET_ERROR_NONE on success
 * \return SOCKET_ERROR_UNKNOWN if address can't be read
 */
socket_error_t nanostack_get_router_ip_address(char *address, int8_t len);

/*
 * \brief Nanostack event handling. Stack is using events in the background and
 * application needs to call this function constantly to keep stack running.
 */
void nanostack_run(void);

#ifdef __cplusplus
}
#endif
#endif // MBED_NANOSTACK_INIT_H_
