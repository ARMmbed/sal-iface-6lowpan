# mbed-6lowpan-adapter
The mbed-6lowpan-adapteris a socket adaptation layer for mesh sockets. 
Applications can to use mesh sockets via mbed C++ socket API.

## Usage Notes
Applications are not allowed to use this module directly, mbed C++ socket API 
should be used. 

Before using the mesh sockets a mesh interface needs to be created and 
initialized by using mbed-mesh-api.

This module is under construction and therefore some limitations exists:

* No DNS resolving, IPv6 address must be used in ascii format
* TCP socket is not working properly
* UDP maximum datagram size id 1280 bytes
* UDP socket can't be connected with `socket_connect()`
* `socket_bind()` binds port only, address must be set to IN_ANY

## Getting Started
Module contains example applications in test folder:

* `nanostack_udp` contains a UDP datagram example application
* `nanostack_tcp` contains TCP socket example application
* `system_test` contains test application

