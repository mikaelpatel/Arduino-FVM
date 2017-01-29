/**
 * @file FVM/Interpret.ino
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
 *
 * @section Words
 *
 * variable x ( -- a-addr ) variable example.
 * -42 constant y ( -- -42) constant example.
 * create pad 32 allot ( -- a-addr ) create-allot example.
 *
 * (array) ( n -- a-addr ) array object handler.
 * (2constant) ( -- x1 x2 ) double constant handler.
 *
 * 4 array z ( n -- a-addr ) array example.
 * 1 2 2constant c2 ( -- 1 2 ) double cell constant example.
 *
 * 1 constant OUTPUT ( -- mode ) output pin mode.
 * 13 constant LED ( -- pin ) built-in led pin.
 * : blink ( ms pin n -- ) blink pin with ms delay n-times.
 *
 * external numbers ( -- 1 2 3 a-addr +n ) external function example.
 */

#include "FVM.h"

// : array ( n -- ) create cells allot
// does> ( n -- a-addr ) swap cells plus ;
FVM_COLON(0, ARRAY, "(array)")
  FVM_OP(DOES),
  FVM_OP(SWAP),
  FVM_OP(CELLS),
  FVM_OP(PLUS),
  FVM_OP(EXIT)
};

// : 2constant ( x1 x2 -- ) create swap , ,
// does> ( -- x1 x2 ) dup @ swap cell + @ ;
FVM_COLON(1, TWO_CONSTANT, "(2constant)")
  FVM_OP(DOES),
  FVM_OP(DUP),
  FVM_OP(FETCH),
  FVM_OP(SWAP),
  FVM_OP(CELL),
  FVM_OP(PLUS),
  FVM_OP(FETCH),
  FVM_OP(EXIT)
};

// variable x
// 42 x !
int x = 42;
FVM_VARIABLE(2, X, x);

// -42 constant y
FVM_CONSTANT(3, Y, "y", -42);

// 4 array z
int z[] = { 1, 2, 4, 8 };
FVM_CREATE(4, Z, ARRAY, z);

// 1 2 2constant c2
int c2[] = { 1, 2 };
FVM_CREATE(5, C2, TWO_CONSTANT, c2);

// create pad 32 allot
const int PAD_MAX = 32;
char pad[PAD_MAX];
FVM_VARIABLE(6, PAD, pad);

// external void numbers ( -- 1 2 3 a-addr +n )
void numbers(FVM::task_t &task, void* env)
{
  task.push(1);
  task.push(2);
  task.push(3);
  task.push((FVM::cell_t) env);
  task.push(task.depth());
}
FVM_FUNCTION(7, NUMBERS, numbers, pad);

// 1 constant OUTPUT
FVM_CONSTANT(8, OUTPUT_MODE, "OUTPUT", 1);

// 13 constant LED
FVM_CONSTANT(9, LED_PIN, "LED", 13);

// : blink ( ms pin n -- ) 0 do dup digitaltoggle over delay loop 2drop ;
FVM_COLON(10, BLINK, "blink")
  FVM_OP(ZERO),
  FVM_OP(DO), 7,
    FVM_OP(DUP),
    FVM_OP(DIGITALTOGGLE),
    FVM_OP(OVER),
    FVM_OP(DELAY),
  FVM_OP(LOOP), -5,
  FVM_OP(TWO_DROP),
  FVM_OP(EXIT)
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) ARRAY_CODE,
  (code_P) TWO_CONSTANT_CODE,
  (code_P) &X_VAR,
  (code_P) &Y_CONST,
  (code_P) &Z_VAR,
  (code_P) &C2_VAR,
  (code_P) &PAD_VAR,
  (code_P) &NUMBERS_FUNC,
  (code_P) &OUTPUT_MODE_CONST,
  (code_P) &LED_PIN_CONST,
  (code_P) BLINK_CODE,
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) ARRAY_PSTR,
  (str_P) TWO_CONSTANT_PSTR,
  (str_P) X_PSTR,
  (str_P) Y_PSTR,
  (str_P) Z_PSTR,
  (str_P) C2_PSTR,
  (str_P) PAD_PSTR,
  (str_P) NUMBERS_PSTR,
  (str_P) OUTPUT_MODE_PSTR,
  (str_P) LED_PIN_PSTR,
  (str_P) BLINK_PSTR,
  0
};

const int DATA_MAX = 1024;
uint8_t data[1024];

FVM fvm(data, DATA_MAX);
FVM::Task<32,16> task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Interpret: started [Newline]"));
}

void loop()
{
  fvm.interpret(task);
}
