# UDP Example
This application sends data packet to mbed 6LoWPAN gateway that echoes data packet back to the client.

This example is implemented as a logic class (UDPTest) wrapping a UDP socket. The logic class handles all UDP data sending and receiving. Main program will trace the sent and received data when response is received.

## Pre-requisites
To build and run this example the requirements below are necessary:
* A computer with the following software installed
  * CMake
  * yotta
  * python
  * arm gcc toolchain
  * a serial terminal emulator (e.g. screen, pyserial, cu)
  * optionally, for debugging, pyOCD and pyUSB
* A frdm-k64f development board
* A mbed 6LoWPAN shield
* A mbed 6LoWPAN Gateway
* A micro-USB cable
* A micro-USB charger for mbed 6LoWPAN Gateway

## Getting Started
1. Connect the frdm-k64f and 6LoWPAN RF shield together
2. Flash mbed 6LoWPAN gateway using the same RF channel that will be used in the RF shield. File
 node_tasklet.c contains channel definitions (channel list). 
3. Open a terminal in the root mbed-6lowpan-adapter directory
4. Check that there are no missing dependencies

    ```
    $ yt ls
    ```

5. Build the examples. This will take a long time if it is the first time that the examples have been built.

    ```
    $ yt build
    ```

6. Start mbed 6LoWPAN Gateway by connecting USB charger 

7. Copy `build/frdm-k64f-gcc/test/mbed-6lowpan-adaptor-test-nanostack_udp.bin` to your mbed board and wait until the LED next to the USB port stops blinking.

8. Start the serial terminal emulator and connect to the virtual serial port presented by frdm-k64f. For settings, use 9600 baud, 8N1, no flow control.

9. Press the reset button on the board.

10. The output in the terminal window should look like:

    ```
    [INFO][main_appl]: NanoStack UDP Example, stack initialized
    [INFO][node_tasklet]: 6LoWPAN IP Bootstrap started
    ```

## Using a debugger
Optionally, connect using a debugger to set breakpoints and follow program flow. Proceed normally up to and including step 7, then:

1. Open a new terminal window, then start the pyOCD GDB server.

    ```
    $ python pyOCD/test/gdb_server.py
    ```

    The output should look like this:

    ```
    Welcome to the PyOCD GDB Server Beta Version
    INFO:root:new board id detected: 02400201B1130E4E4CXXXXXX
    id => usbinfo | boardname
    0 => MB MBED CM (0xd28, 0x204) [k64f]
    INFO:root:DAP SWD MODE initialised
    INFO:root:IDCODE: 0x2BA01477
    INFO:root:K64F not in secure state
    INFO:root:6 hardware breakpoints, 4 literal comparators
    INFO:root:4 hardware watchpoints
    INFO:root:CPU core is Cortex-M4
    INFO:root:FPU present
    INFO:root:GDB server started at port:3333
    ```

2. Open a new terminal window, go to the root directory of your copy of mbed-6lowpan-adapter, then start GDB and connect to the GDB server.

    ```
    $ arm-none-eabi-gdb -ex "target remote localhost:3333" -ex load ./build/frdm-k64f-gcc/test/mbed-6lowpan-adaptor-test-nanostack_udp
    ```

3. In a third terminal window, start the serial terminal emulator and connect to the virtual serial port presented by frdm-k64f.

4. Once the program has loaded, start it.

    ```
    (gdb) c
    ```

5. The output in the terminal window should look like in step 10 above.
