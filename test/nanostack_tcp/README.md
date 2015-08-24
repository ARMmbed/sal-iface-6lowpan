# TCP Example
This application sends data packet to a remote server that echoes data packet back to the client. 
The remote server needs to be running on a PC that is connected to mbed OS Gateway. 

Remote server IPv6 address must be updated to macro HOST_ADDR in file `main.cpp`.

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
* A test PC running TCP test server 

## Set up remote TCP server
* Set the test PC to run on IP address `fd00:ff1:ce0b:a5e0::1` and use 64-bit network mask `fd00:ff1:ce0b:a5e0::1/64`.
* Power up the mbed 6LoWPAN gateway by connecting usb charger to it
* Connect the mbed 6LoWPAN gateway to the test PC with ethernet cable 
* Copy TCP test server from `test/host_tests/TCP_TestServer_IPv6.py` to the remote PC
* Start server by running command `python TCP_TestServer_IPv6.py`   
* Copy IPv6 address of the test PC to the main application macro `HOST_ADDR`.

## Getting Started
1. Connect the frdm-k64f and 6LoWPAN RF shield together
2. Flash mbed 6LoWPAN gateway using the same RF channel that is used in the RF shield.
3. Open a terminal in the root of `sal-iface_6lowpan` directory
4. Check that there are no missing dependencies

    ```
    $ yt ls
    ```

5. Build the examples. This will take a long time if it is the first time that the examples have been built.

    ```
    $ yt build
    ``` 
6. Copy `build/frdm-k64f-gcc/test/sal-iface-6lowpan-test-nanostack_tcp.bin` to your mbed board and wait until the LED next to the USB port stops blinking.

7. Start the serial terminal emulator and connect to the virtual serial port presented by frdm-k64f. For settings, use 115200 baud, 8N1, no flow control.

8. Press the reset button on the board.

9. The output in the serial terminal emulator window displays the trace messages.

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
    $ arm-none-eabi-gdb -ex "target remote localhost:3333" -ex load ./build/frdm-k64f-gcc/test/sal-iface-6lowpan-test-nanostack_tcp
    ```

3. In a third terminal window, start the serial terminal emulator and connect to the virtual serial port presented by frdm-k64f.

4. Once the program has loaded, start it.

    ```
    (gdb) c
    ```

5. The output in the serial terminal emulator displays trace messages.
