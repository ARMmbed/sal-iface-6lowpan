# System test
This is system test application that can be flashed to the frdm-k64f 
development board. This application will send messages to test server running 
on a host computer and verify that responses are received as expected.

frdm-k64f development board has air interface to 6LoWPAN gateway, The 6LoWPAN 
gateway is connected to the host computer with a network cable.

frdm-k64f <=RF channel=> 6LoWPAN Gateway <=Network cable=> host computer

## Pre-requisites
To build and run this application the requirements below are necessary:
* A computer with the following software installed
  * CMake
  * yotta
  * python
  * arm gcc toolchain
  * a serial terminal emulator (e.g. screen, pyserial, cu)
  * optionally, for debugging, pyOCD and pyUSB
  * optionally, wireshark for tracing IP traffic
* A frdm-k64f development board
* A mbed 6LoWPAN shield
* A mbed 6LoWPAN Gateway
* A micro-USB cable
* A micro-USB charger for mbed 6LoWPAN Gateway
* A network cable to connect mbed 6LoPPAN gateway to the host computer
* Optionally, a separate host computer for running python test servers

## Setting up 6LoWPAN Gateway and Test Server
1. Flash mbed 6LoWPAN gateway. make sure it is configured to the same RF channel that will be used in the mbed 6LoWPAN shield. 
File node_tasklet.c contains channel definitions (channel list).
2. Connect mbed 6LoWPAN Gateway to the host computer with a network cable
3. Start mbed 6LoWPAN Gateway by connecting USB charger
4. Start python test servers (found from ./test/host_tests) in the host computer
5. Check host computer IPv6 address by using ipconfig. Ethernet adapter Local Area Connection 
contains IPv6 address that needs to be updated to the system test application.

## Setting up frdm-k64f development board
1. Connect the frdm-k64f and 6LoWPAN RF shield together
2. Connect computer to frdm-k64f using micro USB cable
3. Modify file main.cpp. Copy/replace test server IPv6 address to macro TEST_SERVER.
4. Build the application and flash it to the development board

## Running the test
1. Start serial terminal emulator to the development board to check traces
2. Press reset button in development board to start the test
3. Check traces in test server and frdm-k64f to see test progress
4. In success case, end of frdm-k64f trace contains lines: {{success}} {{end}}
5. In case frdm-k64f tracing doesn't start, use debugger to reset the board and start tracing again
