/**
 * @file FVM/Shell.ino
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
 * Basic interactive shell with the Forth Virtual Machine (FVM).
 * Example of bindling variable, constant and object handler.
 */

#include "FVM.h"

// Forward declare array handler function
const int ARRAY_FN = 4;
const int TWO_CONST_FN = 5;

// Declare a variable (sketch data reference)
int X = 42;
const int X_ID = 0;
FVM_VARIABLE(X);

// Declare a constant (16-bit value)
const int Y_ID = 1;
FVM_CONSTANT(Y, -42);

// Declare an array with handler function
int Z[] = { 1, 2, 4, 8 };
const int Z_ID = 2;
FVM_CREATE(Z, ARRAY_FN);

// Declare double constant with handler function
int X2[] = { 1, 2 };
const int X2_ID = 3;
FVM_CREATE(X2, TWO_CONST_FN);

// Array handler function (does code)
const char ARRAY_PSTR[] PROGMEM = "array";

// does: array ( index array-addr -- element-addr ) swap cells + ;
const FVM::code_t ARRAY_CODE[] PROGMEM = {
  FVM_OP(DOES),
  FVM_OP(SWAP),
  FVM_OP(CELLS),
  FVM_OP(PLUS),
  FVM_OP(HALT)
};

// Double constant handler function (does code)
const char TWO_CONST_PSTR[] PROGMEM = "2const";

// does: 2const ( addr -- x y ) dup @ swap cell + @ ;
const FVM::code_t TWO_CONST_CODE[] PROGMEM = {
  FVM_OP(DOES),
  FVM_OP(DUP),
  FVM_OP(FETCH),
  FVM_OP(SWAP),
  FVM_OP(CELL),
  FVM_OP(PLUS),
  FVM_OP(FETCH),
  FVM_OP(HALT)
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) &X_VAR,
  (code_P) &Y_CONST,
  (code_P) &Z_VAR,
  (code_P) &X2_VAR,
  (code_P) ARRAY_CODE,
  (code_P) TWO_CONST_CODE
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) X_PSTR,
  (str_P) Y_PSTR,
  (str_P) Z_PSTR,
  (str_P) X2_PSTR,
  (str_P) ARRAY_PSTR,
  (str_P) TWO_CONST_PSTR,
  0
};

// Data area for the shell (if needed)
uint8_t data[128];

// Forth Virtual Machine and Task
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
  // Scan buffer for a single word or number
  char buffer[32];
  char pad[32];
  char* bp = buffer;
  char c;

  // Skip white space
  do {
    while (!Serial.available());
    c = Serial.read();
  } while (c <= ' ');

  // Scan until white space
  do {
    *bp++ = c;
    while (!Serial.available());
    c = Serial.read();
   } while (c > ' ');
  *bp = 0;

  // Check for operation/function name
  if (buffer[0] == '\\') {
    strcpy(pad, buffer+1);
    task.push((int) pad);
  }
  // Lookup and execute
  else
    fvm.execute(buffer, task);

  // Print stack contents after each command line
  if (c == '\n' && !task.trace())
    fvm.execute(FVM::OP_DOT_S, task);
}
