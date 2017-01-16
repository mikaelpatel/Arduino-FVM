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

FVM_COLON(0, ARRAY, "(array)")
  FVM_OP(DOES),
  FVM_OP(SWAP),
  FVM_OP(CELLS),
  FVM_OP(PLUS),
  FVM_OP(EXIT)
};

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

int x = 42;
FVM_VARIABLE(2, X, x);

FVM_CONSTANT(3, Y, "y", -42);

int z[] = { 1, 2, 4, 8 };
FVM_CREATE(4, Z, ARRAY, z);

int c2[] = { 1, 2 };
FVM_CREATE(5, C2, TWO_CONST, c2);

const int PAD_MAX = 32;
char pad[PAD_MAX];
FVM_VARIABLE(6, PAD, pad);

void numbers(FVM::task_t &task, void* env)
{
  task.push(1);
  task.push(2);
  task.push(3);
  task.push((FVM::cell_t) env);
  task.push(task.depth());
}
FVM_FUNCTION(7, NUMBERS, numbers, pad);

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

uint8_t data[128];

FVM fvm(data);
FVM::Task<32,16> task(Serial);

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
