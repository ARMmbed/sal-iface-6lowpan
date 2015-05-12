/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
/*
 * Nanostack socket adaptation.
 *
 * Additional layer between mbed socket API and NanoStack socket API was needed
 * to avoid function naming conflicts.
 *
 * Socket adaptation layer (SAL) will call NanoStack methods via this wrapper.
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
extern int8_t nanostack_socket_init_impl(void);
extern int8_t nanostack_get_ip_address_impl(char *address, int8_t len);
extern int8_t nanostack_get_router_ip_address_impl(char *address, int8_t len);
extern int8_t nanostack_socket_network_connect_impl(func_cb_t callback);
extern sock_data_s* nanostack_socket_open_impl(int8_t socket_type, int8_t identifier, void *context);
extern void nanostack_release_socket_data_impl(sock_data_s *sock_data_ptr);
extern int8_t nanostack_socket_free_impl(sock_data_s *sock_data_ptr);
extern int8_t nanostack_socket_close_impl(sock_data_s *sock_data_ptr);
extern int8_t nanostack_socket_connect_impl(sock_data_s *sock_data_ptr, ns_address_t *address);
extern int8_t nanostack_socket_send_impl(sock_data_s *sock_data_ptr, uint8_t *buffer, uint16_t length);
extern int8_t nanostack_socket_send_to_impl(sock_data_s *sock_data_ptr, ns_address_t *address, uint8_t *buffer, uint16_t length);
extern int8_t nanostack_socket_recv_from_imp(sock_data_s *sock_data_ptr, ns_address_t *addr, uint8_t *buffer, uint16_t length);
extern void nanostack_run_impl(void);

#ifdef __cplusplus
}
#endif
#endif
