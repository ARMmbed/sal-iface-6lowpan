/*
 * Copyright (c) 2015 ARM. All rights reserved.
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
typedef struct sock_data_
{
    int8_t socket_id;           /*!< allocated socket ID */
    int8_t security_session_id; /*!< Not used yet */
} sock_data_s;

/*
 * Nanostack wrapper functions
 */
/*
 * \brief NanoStack initialization routine
 * \return >= 0 on success
 * \return negative number on error
 */
extern int8_t ns_wrapper_socket_init(void);

/*
 * \brief Get own IP address
 */
extern int8_t ns_wrapper_get_ip_address(char *address, int8_t len);

/*
 * \brief Get border router IP address.
 */
extern int8_t ns_wrapper_get_router_ip_address(char *address, int8_t len);

/*
 * \brief Connect NanoStack to mesh network
 * \return 0 on success
 * \return >0 on error
 */
extern int8_t ns_wrapper_socket_network_connect(func_cb_t callback);

/*
 * \brief Open NanoStack socket
 */
extern sock_data_s* ns_wrapper_socket_open(int8_t socket_type, int8_t identifier, void *context);

/*
 * \brief Bind NanoStack socket
 */
extern int8_t ns_wrapper_socket_bind(sock_data_s *sock_data_ptr, ns_address_t *address);

/*
 * \brief Release data allocated by NanoStack socket
 */
extern void ns_wrapper_release_socket_data(sock_data_s *sock_data_ptr);

/*
 * \brief Free resources allocated by NanoStack socket
 */
extern int8_t ns_wrapper_socket_free(sock_data_s *sock_data_ptr);

/*
 * \brief Close NanoStack socket
 */
extern int8_t ns_wrapper_socket_close(sock_data_s *sock_data_ptr);

/*
 * \brief Connect NanoStack socket
 */
extern int8_t ns_wrapper_socket_connect(sock_data_s *sock_data_ptr, ns_address_t *address);

/*
 * \brief Send data to NanoStack socket
 */
extern int8_t ns_wrapper_socket_send(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length);

/*
 * \brief Send data to NanoStack socket
 */
extern int8_t ns_wrapper_socket_send_to(sock_data_s *sock_data_ptr, ns_address_t *address, uint8_t *buffer, uint16_t length);

/*
 * \brief Receive data from NanoStack socket
 */
extern int8_t ns_wrapper_socket_recv_from(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length);

/*
 * \brief Run NanoStack event loop
 */
extern void ns_wrapper_run(void);

#ifdef __cplusplus
}
#endif
#endif
