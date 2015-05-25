/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#ifndef _NS_SAL_UTILS_H_
#define _NS_SAL_UTILS_H_

/*
 * \brief Convert mbed socket address to NanoStack address
 * \param ns_addr nanoStack socket address astructure
 * \param s_addr mbed socket address structure
 * \param port
 */
void convert_mbed_addr_to_ns(ns_address_t *ns_addr, const struct socket_addr *s_addr, uint16_t port);

/*
 * translate mbed socket address to NanoStack address structure.
 */
/*
 * \brief Convert NanoStack socket address to mbed address
 * \param s_addr mbed socket address structure
 * \param ns_addr nanoStack socket address astructure
 * \param port
 */
void convert_ns_addr_to_mbed(struct socket_addr *s_addr, const ns_address_t *ns_addr, uint16_t *port);

#endif /* _NS_SAL_UTILS_H_ */
