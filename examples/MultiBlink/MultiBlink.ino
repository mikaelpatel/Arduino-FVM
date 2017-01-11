/**
 * @file FVM/MultiBlink.ino
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
 * Classical blink sketch using the Forth Virtual Machine (FVM)
 * with multiple tasks.
 */

#include <FVM.h>

#define USE_TRACE

// 1 constant OUTPUT
FVM_CONSTANT(0, OUTPUT_MODE, "OUTPUT", 1);

// : blink ( ms pin -- )
//   OUTPUT over pinmode
//   begin
//     dup digitaltoggle
//     over delay
//   again ;
FVM_COLON(1, BLINK, "blink")
  FVM_CALL(OUTPUT_MODE),
  FVM_OP(OVER),
  FVM_OP(PINMODE),
    FVM_OP(DUP),
    FVM_OP(DIGITALTOGGLE),
    FVM_OP(OVER),
    FVM_OP(DELAY),
  FVM_OP(BRANCH), -5,
  FVM_OP(EXIT)
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) &OUTPUT_MODE_CONST,
  BLINK_CODE
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) OUTPUT_MODE_PSTR,
  (str_P) BLINK_PSTR,
  0
};

FVM::Task<16,8> task1(Serial);
FVM::Task<16,8> task2(Serial);
FVM fvm;

void setup()
{
  Serial.begin(57600);
  while (!Serial);

#if defined(USE_TRACE)
  task1.trace(1);
  task2.trace(1);
#endif

  // 500 13 blink
  task1.push(500);
  task1.push(13);
  fvm.execute(BLINK_CODE, task1);

  // 100 12 blink
  task2.push(100);
  task2.push(12);
  fvm.execute(BLINK_CODE, task2);
}

void loop()
{
  fvm.resume(task1);
  fvm.resume(task2);
}
