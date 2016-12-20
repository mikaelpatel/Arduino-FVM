/**
 * @file FVM/Blink.ino
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2016, Mikael Patel
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
 * @section Foot-print
 *  Bytes      Section
 * ---------------------------------------
 *        1482 Main and Serial
 *  +72   1554 +Blink sketch code and task
 *  +3500 5054 +Forth Virtual Machine
 *  +776  5830 +Kernel dictionary
 *  +506  6326 +Trace mode
 * ---------------------------------------
 * Extra bytes when adding C++ code for:
 *  +2    6328 hex
 *  +2    6328 decimal
 *  +4    6330 here
 *  +4    6330 ,
 *  +6    6332 allot
 *  +6    6332 ,
 *  +10   6336 +!
 *  +10   6336 ?dup
 *  +10   6336 -rot
 *  +12   6338 min
 *  +12   6338 max
 *  +14   6340 <>
 *  +14   6340 <
 *  +14   6340 =
 *  +14   6340 >
 *  +36   6362 within
 *  +410  6736 .
 *  +476  6802 .s
 * ---------------------------------------
 *  Arduino Uno/IDE 1.6.13
 */

#if 1
#include "FVM.h"

const int BLINK_FN = 0;
const char BLINK_PSTR[] PROGMEM = "blink";
// : blink ( ms pin -- )
//   1 over pinmode
//   begin
//     dup digitaltoggle
//     over delay
//   repeat
// ;
const FVM::code_t BLINK_CODE[] PROGMEM = {
  FVM_OP(ONE),
  FVM_OP(OVER),
  FVM_OP(PINMODE),
  FVM_OP(DUP),
  FVM_OP(DIGITALTOGGLE),
  FVM_OP(OVER),
  FVM_OP(DELAY),
  FVM_OP(BRANCH), -6,
  FVM_OP(EXIT)
};

const int SKETCH_FN = 1;
const char SKETCH_PSTR[] PROGMEM = "sketch";
// : sketch ( -- ) 500 13 blink ;
const FVM::code_t SKETCH_CODE[] PROGMEM = {
  FVM_LIT(500),
  FVM_CLIT(13),
  FVM_CALL(BLINK_FN),
  FVM_OP(HALT)
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  BLINK_CODE,
  SKETCH_CODE
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) BLINK_PSTR,
  (str_P) SKETCH_PSTR,
  0
};

FVM fvm;
FVM::task_t task(Serial);
#endif

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Blink: started"));
}

void loop()
{
#if 1
  fvm.execute(SKETCH_CODE, task);
#endif
}
