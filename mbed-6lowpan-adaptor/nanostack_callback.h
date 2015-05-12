/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * NanoStack socket callbacks.
 */

#ifndef _NANOSTACK_CALLBACK_H
#define _NANOSTACK_CALLBACK_H

extern void nanostack_data_received_callback(void* context);
extern void nanostack_tx_done_callback(void* context, uint16_t length);
extern void nanostack_tx_error_callback(void* context);
extern void nanostack_connect_callback(void* context);

#endif /* _NANOSTACK_SOCKET_H */
