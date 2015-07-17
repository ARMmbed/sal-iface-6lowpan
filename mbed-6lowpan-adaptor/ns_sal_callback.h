/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#ifndef _NS_SAL_CALLBACK_H_
#define _NS_SAL_CALLBACK_H_
/*
 * Buffer for the received datagram.
 * Every socket will contain received data in linked list.
 */
typedef struct _data_buff_t
{
    struct _data_buff_t *next;  /*<! next buffer */
    ns_address_t ns_address;    /*<! address where data is received */
    uint16_t length;            /*<! data length in this buffer */
    uint8_t payload[];          /*<! Trailing buffer data */
} data_buff_t;


/*
 * \brief address resolved callback
 * \param context context that receives name resolving result
 * \param address to resolve
 */
void ns_sal_callback_name_resolved(void* context, const char *address);

/*
 * \brief Data received callback
 * \param context context that receives data
 * \param data_buf received data, can be NULL
 */
void ns_sal_callback_data_received(void* context, data_buff_t *data_buf);

/*
 * \brief Data has been transmitted callback
 * \param context context that sends data
 * \param length number of bytes transmitted
 */
void ns_sal_callback_tx_done(void* context, uint16_t length);

/*
 * \brief Data transmission error callback
 * \param context sending data
 */
void ns_sal_callback_tx_error(void* context);

/*
 * \brief Socket connected callback
 * \param context connected
 */
void ns_sal_callback_connect(void* context);

/*
 * \brief Socket disconnected callback
 * \param context disconnected
 */
void ns_sal_callback_disconnect(void* context);

#endif /* _NS_SAL_CALLBACK_H_ */
