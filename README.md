# Arduino-FVM

This library provides a token threaded Forth kernel that may be
embedded in Arduino sketches. The kernel can be used as a debug
shell or even full scale Forth applications.

![shell-screenshot](https://dl.dropboxusercontent.com/u/993383/Cosa/screenshots/Screenshot%20from%202016-12-30%2020-40-06.png)

The Forth Virtual Machine allows multi-tasking which makes it easy to
integrate with the Arduino core library functions. Context switch to
and from the virtual machine is as low as 6.4 us (halt) and 9.3 us
(yield/branch).

![compiler-screenshot](https://dl.dropboxusercontent.com/u/993383/Cosa/screenshots/Screenshot%20from%202017-01-01%2016-54-07.png)

A code generator, token compiler, example sketch is available. Forth
declarations are translated to the Forth Virtual Machine instruction
set and dictionary format. The token compiler is also an excellent
example of mixing forth, C/C++ and Arduino library functions in the
same sketch.

The Forth Virtual Machine (FVM) with 130 instructions is approx. 4.5
Kbyte without kernel dictionary table and strings. This adds approx. 1
Kbyte. The instruction level trace adds an additional 500 bytes. Many of
the kernel instructions are defined in both C++ and FVM
instructions. This allows tailoring for speed and/or size.

## Tokens

The Forth Virtual Machine is byte token threaded. Most kernel and
application definitions are a single byte. A prefix token is used to
extend beyond 256 tokens.

A total of 512 tokens are allowed; 0..255 are kernel tokens and
256..511 are application tokens. Kernel tokens are operations
codes. These can be C++ code and/or threaded code (in FVM.cpp). The
application tokens are threaded code.

Kernel tokens 0..127 are direct operation codes while tokens 128..255
require a prefix (OP_KERNEL).

Application tokens 256..383 (-1..-128) are directly nested by the
kernel. The token value are the one-complement index for a threaded
code table in program memory.

Application tokens 384..511 require a prefix (OP_CALL) to the mapped
values 0..127 which are the index for a threaded code table in data
memory.

## Optimizations

The token threading inner interpreter uses serial optimizations to
reduce call graphs.

First optimization, the _token mapping_ (-1..-128) allows the inner
interpreter to directly handle call of threaded code. The inner
interpreter will call threaded code without extra dispatching to
nesting.

Second optimization, _tail call reduction_, allows the inner interpreter
to replace calls with jumps when the preceeding operation is
EXIT. This reduces return stack depth, pushing and poping return
address.

## Install

Download and unzip the Arduino-FVM library into your sketchbook
libraries directory. Rename from Arduino-FVM-master to FVM.

The FVM library and examples should be found in the Arduino IDE
File>Examples menu.
