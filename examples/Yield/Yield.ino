/**
 * @file FVM/Yield.ino
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2017, Mikael Patel
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
 * Measure the Forth Virtual Machine (FVM) context switch
 * performance.
 *
 * @section Measurements
 * Context switch time to and from the virtual machine
 * measured in microseconds for each trace mode (0, 1, 2)
 * and threading (0).
 *
 * Arduino Uno
 * Halt: 8, 11, 16 us
 * Yield/branch: 10, 18, 23 us
 *
 * Arduino Due
 * Halt: 1.42, 2.55, 3.95 us
 * Yield/branch: 2.18, 4.20, 5.84 us
 *
 * @section Environment
 * Arduino Uno/IDE 1.8.1
 */

#include "FVM.h"

#if 0
// : sketch ( -- ) halt ;
FVM_COLON(0, SKETCH, "sketch")
  FVM_OP(HALT),
  FVM_OP(EXIT)
};
#else
// : sketch ( -- ) begin yield again ;
FVM_COLON(0, SKETCH, "sketch")
  FVM_OP(YIELD),
  FVM_OP(BRANCH), -2,
  FVM_OP(EXIT)
};
#endif

// Sketch function table
const FVM::code_P FVM::fntab[] PROGMEM = {
  SKETCH_CODE
};

// Sketch symbol table
const str_P FVM::fnstr[] PROGMEM = {
  (str_P) SKETCH_PSTR,
  0
};

FVM::Task<32,16> task(Serial, SKETCH_CODE);
FVM fvm;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
}

void loop()
{
  int i = 1000;
  uint32_t start = micros();
  while (--i) fvm.resume(task);
  uint32_t stop = micros();
  float us = (stop-start) / 1000.0;
  Serial.print(us);
  Serial.println(F(" us"));
  Serial.flush();
  delay(100);
}
