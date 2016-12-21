/**
 * @file FVM/Demo.ino
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
 * A demo sketch for the Forth Virtual Machine (FVM).
 */

#include "FVM.h"

const int SKETCH_FN = 0;
const char SKETCH_PSTR[] PROGMEM = "sketch";
const FVM::code_t SKETCH_CODE[] PROGMEM = {
  // Stack operations
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(TWO),			// 0 1
  FVM_OP(SWAP),			// 0 1 2
  FVM_OP(ROT),			// 0 2 1
  FVM_OP(QDUP),			// 2 1 0
  FVM_OP(OVER),			// 2 1 0
  FVM_OP(TUCK),			// 2 1 0 1
  FVM_OP(QDUP),			// 2 1 1 0 1
  FVM_OP(DROP),			// 2 1 1 0 1 1
  FVM_OP(NIP),			// 2 1 1 0 1
  FVM_OP(DEPTH),		// 2 1 1 1
  FVM_OP(MINUS_ROT),		// 2 1 1 1 4
  FVM_OP(EMPTY),		// 2 1 4 1 1
  FVM_OP(YIELD),		// -

  // Double element operations
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(TWO_DUP),		// 0 1
  FVM_OP(SWAP),			// 0 1 0 1
  FVM_OP(TWO_SWAP),		// 0 1 1 0
  FVM_OP(TWO_OVER),		// 1 0 0 1
  FVM_OP(TWO_DROP),		// 1 0 0 1 1 0
  FVM_OP(EMPTY),		// 1 0 0 1
  FVM_OP(YIELD),		// -

  // Return stack operations
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(TO_R),			// 0 1
  FVM_OP(TO_R),			// 0
  FVM_OP(TWO),			// -
  FVM_OP(R_FETCH),		// 2
  FVM_OP(R_FROM),		// 2 0
  FVM_OP(R_FROM),		// 2 0 0
  FVM_OP(EMPTY),		// 2 0 0 1
  FVM_OP(YIELD),		// -

  // Bit-wise/boolean operations
  FVM_OP(ONE),			// -
  FVM_OP(BOOL),			// 1
  FVM_OP(NOT),			// -1
  FVM_OP(INVERT),		// 0
  FVM_CLIT(-13),		// -1
  FVM_OP(AND),			// -1 -13
  FVM_LIT(10),			// -13
  FVM_OP(OR),			// -13 10
  FVM_CLIT(10),			// -5
  FVM_OP(XOR),			// -5 10
  FVM_OP(EMPTY),		// -15
  FVM_OP(YIELD),		// -

  // Arithmetic operations
  FVM_OP(MINUS_ONE),		// -
  FVM_OP(NEGATE),		// -1
  FVM_OP(ONE_MINUS),		// 1
  FVM_OP(ONE_PLUS),		// 0
  FVM_OP(TWO_MINUS),		// 1
  FVM_OP(TWO_PLUS),		// -1
  FVM_OP(TWO_PLUS),		// 1
  FVM_OP(TWO_STAR),		// 3
  FVM_OP(TWO_SLASH),		// 6
  FVM_OP(ONE),			// 3
  FVM_OP(PLUS),			// 3 1
  FVM_OP(MINUS_ONE),		// 4
  FVM_OP(MINUS),		// 4 -1
  FVM_OP(MINUS_TWO),		// 5
  FVM_OP(MINUS),		// 5 -2
  FVM_OP(MINUS_TWO),		// 7
  FVM_OP(STAR),			// 7 -2
  FVM_CLIT(5),			// -14
  FVM_OP(SLASH_MOD),		// -14 5
  FVM_OP(NEGATE),		// -2 -4
  FVM_OP(LSHIFT),		// -2 4
  FVM_CLIT(5),			// -32
  FVM_OP(RSHIFT),		// -32 5
  FVM_OP(EMPTY),		// -1
  FVM_OP(YIELD),		// -

  // Extended arithmetic operations
  FVM_OP(ONE),			// -
  FVM_OP(ZERO),			// 1
  FVM_OP(TWO),			// 1 0
  FVM_OP(WITHIN),		// 1 0 2
  FVM_OP(ONE),			// -1
  FVM_OP(MIN),			// -1 1
  FVM_OP(ABS),			// -1
  FVM_OP(TWO),			// 1
  FVM_OP(MAX),			// 1 2
  FVM_OP(EMPTY),		// 2
  FVM_OP(YIELD),		// -

  // Relational operations
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO_NOT_EQUALS),	// 0
  FVM_OP(DROP),			// 0
  FVM_OP(ONE),			// -
  FVM_OP(ZERO_NOT_EQUALS),	// 1
  FVM_OP(DROP),			// -1
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO_LESS),		// 0
  FVM_OP(DROP),			// 0
  FVM_OP(ONE),			// -
  FVM_OP(ZERO_LESS),		// 1
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO_EQUALS),		// 0
  FVM_OP(DROP),			// -1
  FVM_OP(ONE),			// -
  FVM_OP(ZERO_EQUALS),		// 1
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO_GREATER),		// 0
  FVM_OP(DROP),			// 0
  FVM_OP(ONE),			// -
  FVM_OP(ZERO_GREATER),		// 1
  FVM_OP(DROP),			// -1
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO),			// 0
  FVM_OP(NOT_EQUALS),		// 0 0
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(NOT_EQUALS),		// 0 1
  FVM_OP(DROP),			// -1
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO),			// 0
  FVM_OP(LESS),			// 0 0
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(LESS),			// 0 1
  FVM_OP(DROP),			// -1
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO),			// 0
  FVM_OP(EQUALS),		// 0 0
  FVM_OP(DROP),			// -1
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(EQUALS),		// 0 1
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ZERO),			// 0
  FVM_OP(GREATER),		// 0 0
  FVM_OP(DROP),			// 0
  FVM_OP(ZERO),			// -
  FVM_OP(ONE),			// 0
  FVM_OP(GREATER),		// 0 1
  FVM_OP(DROP),			// 0
  FVM_OP(YIELD),		// -

  // Delay
  FVM_LIT(100),			// -
  FVM_OP(DELAY),		// 1000
  FVM_OP(HALT)			// -
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  SKETCH_CODE
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) SKETCH_PSTR,
  0
};

FVM fvm;
FVM::task_t task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  Serial.println(F("FVM/Demo: started"));
  Serial.print(F("sizeof(SKETCH_CODE) = "));
  Serial.println(sizeof(SKETCH_CODE));

  fvm.execute(SKETCH_CODE, task);
  while (fvm.resume(task) > 0);
}

void loop()
{
}
