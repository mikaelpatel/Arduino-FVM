/**
 * @file FVM/Compiler.ino
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
 * Basic interactive compiler for the Forth Virtual Machine (FVM).
 * Compiles forth definitions, statements and generates virtual
 * machine code (C++).
 */

#include "FVM.h"

// : mark> ( -- addr ) here 0 c, ;
const int FORWARD_MARK = 0;
const char FORWARD_MARK_PSTR[] PROGMEM = "mark>";
const FVM::code_t FORWARD_MARK_CODE[] PROGMEM = {
  FVM_OP(HERE),
  FVM_OP(ZERO),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

// : resolve> ( addr -- ) here over - swap c! ;
const int FORWARD_RESOLVE = 1;
const char FORWARD_RESOLVE_PSTR[] PROGMEM = "resolve>";
const FVM::code_t FORWARD_RESOLVE_CODE[] PROGMEM = {
  FVM_OP(HERE),
  FVM_OP(OVER),
  FVM_OP(MINUS),
  FVM_OP(SWAP),
  FVM_OP(C_STORE),
  FVM_OP(EXIT)
};

// : <mark ( -- addr ) here ;
const int BACKWARD_MARK = 2;
const char BACKWARD_MARK_PSTR[] PROGMEM = "<mark";
const FVM::code_t BACKWARD_MARK_CODE[] PROGMEM = {
  FVM_OP(HERE),
  FVM_OP(EXIT)
};

// : <resolve ( addr -- ) here - c, ;
const int BACKWARD_RESOLVE = 3;
const char BACKWARD_RESOLVE_PSTR[] PROGMEM = "<resolve";
const FVM::code_t BACKWARD_RESOLVE_CODE[] PROGMEM = {
  FVM_OP(HERE),
  FVM_OP(MINUS),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

// : if ( -- addr ) compile (0branch) mark> ; immediate
const int IF = 4;
const char IF_PSTR[] PROGMEM = "if";
const FVM::code_t IF_CODE[] PROGMEM = {
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
};

// : then ( addr -- ) resolve> ; immediate
const int THEN = 5;
const char THEN_PSTR[] PROGMEM = "then";
const FVM::code_t THEN_CODE[] PROGMEM = {
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : else ( addr1 -- addr2 ) compile (branch) mark> swap resolve> ; immediate
const int ELSE = 6;
const char ELSE_PSTR[] PROGMEM = "else";
const FVM::code_t ELSE_CODE[] PROGMEM = {
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(SWAP),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : begin ( -- addr ) <mark ; immediate
const int BEGIN = 7;
const char BEGIN_PSTR[] PROGMEM = "begin";
const FVM::code_t BEGIN_CODE[] PROGMEM = {
  FVM_CALL(BACKWARD_MARK),
  FVM_OP(EXIT)
};

// : again ( addr -- ) compile (branch) <resolve ; immediate
const int AGAIN = 8;
const char AGAIN_PSTR[] PROGMEM = "again";
const FVM::code_t AGAIN_CODE[] PROGMEM = {
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : until ( addr -- ) compile (0branch) <resolve ; immediate
const int UNTIL = 9;
const char UNTIL_PSTR[] PROGMEM = "until";
const FVM::code_t UNTIL_CODE[] PROGMEM = {
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : while ( addr1 -- addr1 addr2 ) compile (0branch) mark> ; immediate
const int WHILE = 10;
const char WHILE_PSTR[] PROGMEM = "while";
#if 0
const FVM::code_t WHILE_CODE[] PROGMEM = {
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
};
#else
# define WHILE_CODE IF_CODE
#endif

// : repeat (addr1 addr2 -- ) swap [compile] again resolve> ; immediate
const int REPEAT = 11;
const char REPEAT_PSTR[] PROGMEM = "repeat";
const FVM::code_t REPEAT_CODE[] PROGMEM = {
  FVM_OP(SWAP),
  FVM_CALL(AGAIN),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

const FVM::code_P FVM::fntab[] PROGMEM = {
  FORWARD_MARK_CODE,
  FORWARD_RESOLVE_CODE,
  BACKWARD_MARK_CODE,
  BACKWARD_RESOLVE_CODE,
  IF_CODE,
  THEN_CODE,
  ELSE_CODE,
  BEGIN_CODE,
  AGAIN_CODE,
  UNTIL_CODE,
  WHILE_CODE,
  REPEAT_CODE
};

const str_P FVM::fnstr[] PROGMEM = {
  (str_P) FORWARD_MARK_PSTR,
  (str_P) FORWARD_RESOLVE_PSTR,
  (str_P) BACKWARD_MARK_PSTR,
  (str_P) BACKWARD_RESOLVE_PSTR,
  (str_P) IF_PSTR,
  (str_P) THEN_PSTR,
  (str_P) ELSE_PSTR,
  (str_P) BEGIN_PSTR,
  (str_P) AGAIN_PSTR,
  (str_P) UNTIL_PSTR,
  (str_P) WHILE_PSTR,
  (str_P) REPEAT_PSTR,
  0
};

// Data area for the shell
uint8_t data[128];
char pad[32];
bool compiling = false;

// Forth virtual machine and task
FVM fvm(data);
FVM::task_t task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Compiler: started [Newline]"));
}

void loop()
{
  char buffer[32];
  int op;

  // Scan buffer for a single word or number
  scan(buffer);
  op = fvm.lookup(buffer);

  // Check for special words or literals
  if (op < 0) {

    // Check for start of definition
    if (!strcmp_P(buffer, PSTR(":")) && !compiling) {
      scan(buffer);
      strcpy(pad, buffer);
      compiling = true;
    }

    // Check for end of definition
    else if (!strcmp_P(buffer, PSTR(";")) && compiling) {
      fvm.compile(FVM::OP_EXIT);
      codegen();
      fvm.dp(data);
      compiling = false;
    }

    // Assume number
    else {
      int value = atoi(buffer);
      if (compiling) {
	if (value < -128 || value > 127) {
	  fvm.compile(FVM::OP_LIT);
	  fvm.compile(value >> 8);
	  fvm.compile(value);
	}
	else {
	  fvm.compile(FVM::OP_CLIT);
	  fvm.compile(value);
	}
      }
      else {
	task.push(value);
      }
    }
  }

  // Compile operation
  else if (op < 128 && compiling) {
    fvm.compile(op);
  }
  else {
    fvm.execute(op, task);
  }
}

void scan(char* bp)
{
  char c;
  do {
    while (!Serial.available());
    c = Serial.read();
  } while (c <= ' ');
  do {
    *bp++ = c;
    while (!Serial.available());
    c = Serial.read();
   } while (c > ' ');
  *bp = 0;
}

void codegen()
{
  Serial.println(F("const int XXX = N;"));
  Serial.print(F("const char XXX_PSTR[] PROGMEM = \""));
  Serial.print(pad);
  Serial.println(F("\";"));
  Serial.print(F("const FVM::code_t XXX_CODE[] PROGMEM = {\n  "));
  for (uint8_t* dp = data; dp < fvm.dp(); dp++) {
    Serial.print((int8_t) *dp);
    if ((dp + 1) < fvm.dp()) Serial.print(F(", "));
  }
  Serial.println();
  Serial.println(F("};"));
}
