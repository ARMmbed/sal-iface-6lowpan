/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NanoStack Socket Abstraction Layer (SAL) utility functions.
 */

#include <string.h> // memcpy
#include "ns_address.h"
#include "sal/socket_api.h"
#include "sal-iface-6lowpan/ns_sal_utils.h"

void convert_mbed_addr_to_ns(ns_address_t *ns_addr,
                             const struct socket_addr *s_addr, uint16_t port)
{
    ns_addr->type = ADDRESS_IPV6;
    ns_addr->identifier = port;
    memcpy(ns_addr->address, s_addr->ipv6be, 16);
}

void convert_ns_addr_to_mbed(struct socket_addr *s_addr, const ns_address_t *ns_addr,
                             uint16_t *port)
{
    *port = ns_addr->identifier;
    memcpy(s_addr->ipv6be, ns_addr->address, 16);
}
