# mbed-6lowpan-adaptor
The mbed-6lowpan-adaptor is a socket adaptation layer for mesh socket. This module 
enables usage of mesh sockets via [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets).

## Usage Notes
Applications should not use this module directly, [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets) 
should be used instead. 

Before mesh sockets can be used a mesh network needs to be created by using 
[mbed-mesh-api](https://github.com/ARMmbed/mbed-mesh-api).

This module is under construction and therefore some limitations exists:

* No DNS resolving, IPv6 address must be used in ascii format
* UDP maximum datagram size id 1280 bytes
* UDP socket can't be connected with `socket_connect()`
* `socket_bind()` binds port only, address must be set to `IN_ANY`
* No API to set socket options (`set_sock_opt`)
* TCP socket is not working properly and it should not be used
    * `socket_accept` not implemented
    * `socket_start_listen` not implemented
    * `socket_stop_listen` not implemented
    * `is_connected`, `is_bound`, `tx_busy` and `rx_busy` not implemented

## Getting Started
Module contains example applications in test folder:

* `nanostack_udp` contains a UDP datagram example application
* `nanostack_tcp` contains TCP socket example application
* `system_test` contains test application

