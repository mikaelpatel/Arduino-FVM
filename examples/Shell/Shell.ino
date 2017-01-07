/**
 * @file FVM/Shell.ino
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
 * Basic interactive shell with the Forth Virtual Machine (FVM).
 * Example of bindling variable, constant and object handler.
 */

#include "FVM.h"

// does> ( index array-addr -- element-addr ) swap cells + ;
FVM_COLON(0, ARRAY, "(array)")
  FVM_OP(DOES),
  FVM_OP(SWAP),
  FVM_OP(CELLS),
  FVM_OP(PLUS),
  FVM_OP(EXIT)
};

// does> ( addr -- x y ) dup @ swap cell + @ ;
FVM_COLON(1, TWO_CONST, "(2const)")
  FVM_OP(DOES),
  FVM_OP(DUP),
  FVM_OP(FETCH),
  FVM_OP(SWAP),
  FVM_OP(CELL),
  FVM_OP(PLUS),
  FVM_OP(FETCH),
  FVM_OP(EXIT)
};

// variable x 42 x !
int x = 42;
FVM_VARIABLE(2, X, x);

// -42 constant y
FVM_CONSTANT(3, Y, "y", -42);

// create z 1 , 2 , 4 , 8 , does: (array)
int z[] = { 1, 2, 4, 8 };
FVM_CREATE(4, Z, ARRAY, z);

// create c2 1 , 2 , does: (2const)
int c2[] = { 1, 2 };
FVM_CREATE(5, C2, TWO_CONST, c2);

// create pad 32 cells allot
const int PAD_MAX = 32;
char pad[PAD_MAX];
FVM_VARIABLE(6, PAD, pad);

// Extended function call
FVM::data_t* numbers(FVM::data_t* sp)
{
  *++sp = 1;
  *++sp = 2;
  *++sp = 3;
  return (sp);
}
FVM_FUNCTION(7, NUMBERS, numbers);

// Sketch function table
const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) ARRAY_CODE,
  (code_P) TWO_CONST_CODE,
  (code_P) &X_VAR,
  (code_P) &Y_CONST,
  (code_P) &Z_VAR,
  (code_P) &C2_VAR,
  (code_P) &PAD_VAR,
  (code_P) &NUMBERS_FUNC
};

// Sketch symbol table
const str_P FVM::fnstr[] PROGMEM = {
  (str_P) ARRAY_PSTR,
  (str_P) TWO_CONST_PSTR,
  (str_P) X_PSTR,
  (str_P) Y_PSTR,
  (str_P) Z_PSTR,
  (str_P) C2_PSTR,
  (str_P) PAD_PSTR,
  (str_P) NUMBERS_PSTR,
  0
};

// Data area for the shell (if needed)
uint8_t data[128];

// Forth virtual machine and task
FVM fvm(data);
FVM::task_t task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Shell: started [Newline]"));
}

void loop()
{
  fvm.interpret(task);
}
