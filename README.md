# mbed mesh socket
The mbed mesh socket is a socket adaptation layer for a mesh socket. This module 
enables the usage of mesh sockets via [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets).

## Usage notes
Applications should not use this module directly, the [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets) 
should be used instead. 

Before you can use the mesh sockets, you need to create a mesh network using the 
[mbed-mesh-api](https://github.com/ARMmbed/mbed-mesh-api).

This module is under construction and therefore, there are some limitations:

* DNS resolving is not available, IPv6 address must be used in ascii format.
* UDP maximum datagram size is 1280 bytes.
* UDP socket cannot be connected with `socket_connect()`.
* `socket_bind()` binds port only, address must be set to `IN_ANY`.
* There is no API to set the socket options (`set_sock_opt`).
* TCP socket is not working properly and it should not be used
    * `socket_accept`  is not implemented
    * `socket_start_listen` is not implemented
    * `socket_stop_listen` is not implemented
    * `is_connected`, `is_bound`, `tx_busy` and `rx_busy` are not implemented

## Getting started
The module contains the following example applications in the `test` folder:

* `nanostack_udp` contains a UDP datagram example application.
* `nanostack_tcp` contains a TCP socket example application.
* `system_test` contains a test application.

