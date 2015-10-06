# Socket adaptation layer for mesh socket
This module is a socket adaptation layer for a mesh socket. This module 
enables the usage of mesh sockets via [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets).

## Usage notes
Applications should not use this module directly, the [mbed C++ socket API](https://github.com/ARMmbed/mbed-net-sockets) 
should be used instead. 

Before you can use the mesh sockets, you need to create a mesh network using the 
[mbed-mesh-api](https://github.com/ARMmbed/mbed-mesh-api).

This module is under construction and therefore, there are some limitations:

* DNS resolving is not available, IPv6 address must be used in ascii format.
* UDP maximum datagram size is 1280 bytes.
* There is no API to set the socket options (`set_sock_opt`).
* TCP socket does not support method `send_to()` or `recv_from()`
* Following Socket API methods are not implemented:
    * `socket_accept`
    * `socket_start_listen`
    * `socket_stop_listen`
    * `is_bound`
    * `get_local_addr`
    * `get_remote_addr`
    * `get_local_port`
    * `get_remote_port`

## Getting started
The module contains the following example applications in the `test` folder:

* `nanostack_udp` contains a UDP datagram example application.
* `nanostack_tcp` contains a TCP socket example application.
* `system_test` contains a test application.

