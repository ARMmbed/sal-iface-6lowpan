/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */

/*
 * NanoStack Socket Abstraction Layer (SAL) utility functions.
 */

#include <string.h> // memcpy
#include "ns_address.h"
#include <mbed-net-socket-abstract/socket_api.h>
#include "mbed-6lowpan-adaptor/ns_sal_utils.h"

void convert_mbed_addr_to_ns(ns_address_t *ns_addr,
        const struct socket_addr *s_addr, uint16_t port)
{
    ns_addr->type = ADDRESS_IPV6;
    ns_addr->identifier = port;
    memcpy(ns_addr->address, s_addr->storage, 16);
}

void convert_ns_addr_to_mbed(struct socket_addr *s_addr, const ns_address_t *ns_addr,
        uint16_t *port)
{
    *port = ns_addr->identifier;
    if (ADDRESS_IPV4 == ns_addr->type)
    {
        // TODO: Fix address type when it is available in the API
        s_addr->type = SOCKET_STACK_NANOSTACK_IPV6;
    }
    else
    {
        s_addr->type = SOCKET_STACK_NANOSTACK_IPV6;
    }
    memcpy(s_addr->storage, ns_addr->address, 16);
}
