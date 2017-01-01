# Arduino-FVM

This library provides a token threaded Forth kernel that may be
embedded in Arduino sketches. The kernel can be used as a debug
shell or even full scale Forth applications.

![shell-screenshot](https://dl.dropboxusercontent.com/u/993383/Cosa/screenshots/Screenshot%20from%202016-12-30%2020-40-06.png)

The Forth Virtual Machine allows multi-tasking which makes it easy to
integrate with the Arduino core library functions.

![compiler-screenshot](https://dl.dropboxusercontent.com/u/993383/Cosa/screenshots/Screenshot%20from%202017-01-01%2016-54-07.png)

A code generator example sketch is available. Forth declarations are
translated to the Forth Virtual Machine instruction set and dictionary
format.

The Forth Virtual Machine with 120 instructions is approx. 3.5 Kbyte without
kernel dictionary table and strings. This adds approx. 1 Kbyte. The
instruction level trace adds an additional 1 Kbyte. Many of the kernel
instructions are defined in both C++ and FVM instructions. This allows
tailoring for speed and/or size.

## Install

Download and unzip the Arduino-FVM library into your sketchbook
libraries directory. Rename from Arduino-FVM-master to FVM.

The FVM library and examples should be found in the Arduino IDE
File>Examples menu.

## Note

Please note that this is work in progress.
