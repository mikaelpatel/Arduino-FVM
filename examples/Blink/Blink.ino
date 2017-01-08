/**
 * @file FVM/Blink.ino
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2016-2017, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * @section Description
 * Classical blink sketch using the Forth Virtual Machine (FVM).
 *
 * @section Analysis
 *  Bytes      Section
 * ------------------------------------------------
 *        1400 Baseline; Startup and Serial
 *  +474  1874 +Measurement
 *  +86   1960 +Blink sketch code and task
 *  +3766 5726 +Forth Virtual Machine
 *  +896  6622 +Kernel dictionary (128 words)
 *  +998  7620 +Trace mode
 *  +2    7622 +Code generated sketch
 *
 * Cycle time: 32-36 us
 *
 * Context switch to FVM, delay check and context
 * switch back to C++/Arduino. The delay check is:
 * millis r@ - over u< (0branch) yield (branch)
 *
 * @section Environment
 * Arduino Uno/IDE 1.8.0
 */

#define MEASURE
#define BLINK_SKETCH
#define BLINK_TRACE
#define BLINK_RUN
#define CODE_GENERATED

// Enable/disable virtual machine code
#if defined(BLINK_SKETCH)
#include "FVM.h"

// Use code generated or manual coded blink sketch
#if defined(CODE_GENERATED)

/* Source code for token compiler. Note: constants for OUTPUT and LED.
----------------------------------------------------------------------
1 constant OUTPUT
13 constant LED
: blink ( ms pin -- ) begin dup digitaltoggle over delay again ;
: sketch ( -- ) OUTPUT LED pinmode 500 LED blink halt ;
generate-code
----------------------------------------------------------------------
*/

const char WORD0_PSTR[] PROGMEM = "OUTPUT";
const FVM::const_t WORD0_CONST[] PROGMEM = {
  FVM::OP_CONST, 1
};
const char WORD1_PSTR[] PROGMEM = "LED";
const FVM::const_t WORD1_CONST[] PROGMEM = {
  FVM::OP_CONST, 13
};
const char WORD2_PSTR[] PROGMEM = "blink";
const FVM::code_t WORD2_CODE[] PROGMEM = {
  41, 122, 43, 118, 10, -5, 0
};
const char WORD3_PSTR[] PROGMEM = "sketch";
const FVM::code_t WORD3_CODE[] PROGMEM = {
  -1, -2, 119, 2, -12, 1, -2, -3, 125, 0
};
const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) &WORD0_CONST,
  (code_P) &WORD1_CONST,
  WORD2_CODE,
  WORD3_CODE
};
const str_P FVM::fnstr[] PROGMEM = {
  (str_P) WORD0_PSTR,
  (str_P) WORD1_PSTR,
  (str_P) WORD2_PSTR,
  (str_P) WORD3_PSTR,
  0
};

FVM::task_t task(Serial, WORD3_CODE);

#else

// 1 constant OUTPUT
FVM_CONSTANT(0, OUTPUT_MODE, "OUTPUT", 1);

// 13 constant LED
FVM_CONSTANT(1, LED_PIN, "LED", 13);

// : blink ( ms pin -- ) begin dup digitaltoggle over delay again ;
FVM_COLON(2, BLINK, "blink")
    FVM_OP(DUP),
    FVM_OP(DIGITALTOGGLE),
    FVM_OP(OVER),
    FVM_OP(DELAY),
  FVM_OP(BRANCH), -5,
  FVM_OP(EXIT)
};

// : sketch ( -- ) OUTPUT LED pinmode 500 LED blink halt ;
FVM_COLON(3, SKETCH, "sketch")
  FVM_CALL(OUTPUT_MODE),
  FVM_CALL(LED_PIN),
  FVM_OP(PINMODE),
  FVM_LIT(500),
  FVM_CALL(LED_PIN),
  FVM_CALL(BLINK),
  FVM_OP(HALT)
};

// Sketch function table
const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) &OUTPUT_MODE_CONST,
  (code_P) &LED_PIN_CONST,
  BLINK_CODE,
  SKETCH_CODE
};

// Sketch symbol table
const str_P FVM::fnstr[] PROGMEM = {
  (str_P) OUTPUT_MODE_PSTR,
  (str_P) LED_PIN_PSTR,
  (str_P) BLINK_PSTR,
  (str_P) SKETCH_PSTR,
  0
};
FVM::task_t task(Serial, SKETCH_CODE);
#endif

FVM fvm;
#endif

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  // Enable/disable trace
#if defined(BLINK_TRACE)
  task.trace(true);
#endif
}

void loop()
{
  // Enable/disable measurement
#if defined(MEASURE)
  uint32_t start = micros();
#endif

  // Enable/disable resume (base-line foot-print)
#if defined(BLINK_RUN)
  fvm.resume(task);
#endif

  // Enable/disable measurement
#if defined(MEASURE)
  uint32_t stop = micros();
  uint32_t us = stop-start;
  Serial.print(stop);
  Serial.print(':');
  Serial.println(us);
  Serial.flush();
  delay(100);
#endif
}
