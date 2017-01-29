/**
 * @file FVM/Forth.ino
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
 * Basic interactive shell using the Forth Virtual Machine (FVM).
 * Compiles and executes forth definitions in SRAM and PROGMEM.
 *
 * @section Configuration
 * The Forth Virtual Machine (FVM) should be configured as:
 *
 * #define FVM_THREADING 1
 * #define FVM_KERNEL_DICT 1
 *
 * Symbolic trace is optional. See FVM.cpp.
 *
 * @section Words
 *
 * [ ( -- ) stop compile.
 * ] ( -- ) start compile.
 * literal ( n -- ) compile literal.
 *
 * ( COMMENT ) start comment.
 * ." STRING" display string.
 * : NAME ( -- ) start compile of function defintion.
 * ; ( -- ) end compile of function definition.
 * create NAME ( -- ) define word.
 * variable NAME ( -- ) define variable.
 * constant NAME ( value -- ) define constant with given value.
 * forget NAME ( -- ) reset allocation to given name.
 * ' NAME ( -- xt ) lookup word.
 *
 * if ( bool -- ) start conditional block.
 * else ( -- ) end conditional block and start alternative.
 * then ( -- ) end conditional block.
 *
 * begin ( -- ) start iteration block.
 * again ( -- ) end infinite iteration block.
 * until ( bool -- ) end conditional iteration block.
 * while ( bool -- ) start conditional iteration block.
 * repeat ( -- ) end conditional iteration block.
 *
 * do ( high low -- ) start counting iteration block.
 * loop ( -- ) end counting iteration block.
 * +loop ( n -- ) counting iteration block.
 *
 * mark> ( -- addr ) mark forward branch.
 * resolve> ( addr -- ) resolve forward branch.
 * <mark ( -- addr ) mark backward branch.
 * <resolve ( addr -- ) resolve backward branch.
 */

#include "FVM.h"

// : mark> ( -- addr ) here 0 c, ;
FVM_COLON(0, FORWARD_MARK, "mark>")
  FVM_OP(HERE),
  FVM_OP(ZERO),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

// : resolve> ( addr -- ) here over - swap c! ;
FVM_COLON(1, FORWARD_RESOLVE, "resolve>")
  FVM_OP(HERE),
  FVM_OP(OVER),
  FVM_OP(MINUS),
  FVM_OP(SWAP),
  FVM_OP(C_STORE),
  FVM_OP(EXIT)
};

// : <mark ( -- addr ) here ;
FVM_COLON(2, BACKWARD_MARK, "<mark")
  FVM_OP(HERE),
  FVM_OP(EXIT)
};

// : <resolve ( addr -- ) here - c, ;
FVM_COLON(3, BACKWARD_RESOLVE, "<resolve")
  FVM_OP(HERE),
  FVM_OP(MINUS),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

// : if ( -- addr ) compile (0branch) mark> ; immediate
FVM_COLON(4, IF, "if")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
};

// : then ( addr -- ) resolve> ; immediate
const int THEN = 5;
const char THEN_PSTR[] PROGMEM = "then";
#define THEN_CODE FORWARD_RESOLVE_CODE

// : else ( addr1 -- addr2 ) compile (branch) mark> swap resolve> ; immediate
FVM_COLON(6, ELSE, "else")
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
#define BEGIN_CODE BACKWARD_MARK_CODE

// : again ( addr -- ) compile (branch) <resolve ; immediate
FVM_COLON(8, AGAIN, "again")
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : until ( addr -- ) compile (0branch) <resolve ; immediate
FVM_COLON(9, UNTIL, "until")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : while ( addr1 -- addr1 addr2 ) compile (0branch) mark> ; immediate
const int WHILE = 10;
const char WHILE_PSTR[] PROGMEM = "while";
#define WHILE_CODE IF_CODE

// : repeat ( addr1 addr2 -- ) swap [compile] again resolve> ; immediate
FVM_COLON(11, REPEAT, "repeat")
  FVM_OP(SWAP),
  FVM_CALL(AGAIN),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : do ( -- addr1 addr2 ) compile (do) mark> <mark ; immediate
FVM_COLON(12, DO, "do")
  FVM_OP(COMPILE),
  FVM_OP(DO),
  FVM_CALL(FORWARD_MARK),
  FVM_CALL(BACKWARD_MARK),
  FVM_OP(EXIT)
};

// : loop ( addr1 addr2 -- ) compile (loop) <resolve resolve> ; immediate
FVM_COLON(13, LOOP, "loop")
  FVM_OP(COMPILE),
  FVM_OP(LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

// : +loop ( addr1 addr2 -- ) compile (+loop) <resolve resolve> ; immediate
FVM_COLON(14, PLUS_LOOP, "+loop")
  FVM_OP(COMPILE),
  FVM_OP(PLUS_LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

// Sketch dispatched symbols
FVM_SYMBOL(15, LEFT_BRACKET, "[");
FVM_SYMBOL(16, COMMENT, "(");
FVM_SYMBOL(17, DOT_QUOTE, ".\"");
FVM_SYMBOL(18, LITERAL, "literal");
FVM_SYMBOL(19, SEMICOLON, ";");
FVM_SYMBOL(20, RIGHT_BRACKET, "]");
FVM_SYMBOL(21, COLON, ":");
FVM_SYMBOL(22, CREATE, "create");
FVM_SYMBOL(23, VARIABLE, "variable");
FVM_SYMBOL(24, CONSTANT, "constant");
FVM_SYMBOL(25, WORDS, "words");
FVM_SYMBOL(26, FORGET, "forget");
FVM_SYMBOL(27, TICK, "\'");

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
  REPEAT_CODE,
  DO_CODE,
  LOOP_CODE,
  PLUS_LOOP_CODE
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
  (str_P) DO_PSTR,
  (str_P) LOOP_PSTR,
  (str_P) PLUS_LOOP_PSTR,
  (str_P) LEFT_BRACKET_PSTR,
  (str_P) COMMENT_PSTR,
  (str_P) DOT_QUOTE_PSTR,
  (str_P) LITERAL_PSTR,
  (str_P) SEMICOLON_PSTR,
  (str_P) RIGHT_BRACKET_PSTR,
  (str_P) COLON_PSTR,
  (str_P) CREATE_PSTR,
  (str_P) VARIABLE_PSTR,
  (str_P) CONSTANT_PSTR,
  (str_P) WORDS_PSTR,
  (str_P) FORGET_PSTR,
  (str_P) TICK_PSTR,
  0
};

// Size of data area and dynamic dictionary
const int DATA_MAX = (RAMEND - RAMSTART - 1024);
const int DICT_MAX = (RAMEND - RAMSTART) / 64;

// Forth virtual machine, data area and task
uint8_t data[DATA_MAX];
FVM fvm(data, DATA_MAX, DICT_MAX);
FVM::Task<64,32> task(Serial);

// Interpreter state
int compiling = false;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Forth V1.0.1: started [Newline]"));
}

void loop()
{
  char buffer[32];
  int op, val;
  char c;

  // Scan and lookup word
  c = fvm.scan(buffer, task);
  op = fvm.lookup(buffer);

  // Check for literal value (word not found)
  if (op < 0) {
    char* endptr;
    val = strtol(buffer, &endptr, task.m_base == 10 ? 0 : task.m_base);
    if (*endptr != 0) goto error;
    if (compiling)
      fvm.literal(val);
    else
      task.push(val);
  }

  // Check for kernel words; compile or execute
  else if (op < FVM::KERNEL_MAX) {
    if (compiling) {
      fvm.compile(op);
    }
    else {
      execute(op);
    }
  }

  // Skip comments
  else if (op == COMMENT) {
    while (Serial.read() != ')');
  }

  // Check special forms; Interactive mode
  else if (!compiling) {
    switch (op) {
    case RIGHT_BRACKET:
      compiling = true;
      break;
    case COLON:
      c = fvm.scan(buffer, task);
      if (!fvm.create(buffer)) goto error;
      compiling = true;
      break;
    case CREATE:
      c = fvm.scan(buffer, task);
      if (!fvm.create(buffer)) goto error;
      fvm.compile(FVM::OP_VAR);
      break;
    case VARIABLE:
      c = fvm.scan(buffer, task);
      if (!fvm.variable(buffer)) goto error;
      break;
    case CONSTANT:
      val = task.pop();
      c = fvm.scan(buffer, task);
      if (!fvm.constant(buffer, val)) goto error;
      break;
    case WORDS:
      {
	Stream& ios = task.m_ios;
	const char* s;
	int nr = 0;
	fvm.execute(FVM::OP_WORDS, task);
	ios.println();
	while ((s = fvm.name(nr)) != 0) {
	  int len = ios.print(s);
	  if (++nr % 5 == 0)
	    ios.println();
	  else {
	    for (;len < 16; len++) ios.print(' ');
	  }
	}
	if (nr % 5 != 0) ios.println();
      }
      break;
    case FORGET:
      c = fvm.scan(buffer, task);
      op = fvm.lookup(buffer);
      if (!fvm.forget(op)) goto error;
      break;
    case TICK:
      c = fvm.scan(buffer, task);
      op = fvm.lookup(buffer);
      if (op >= LEFT_BRACKET && op <= TICK) goto error;
      task.push(op);
      break;
    default:
      if (op < FVM::APPLICATION_MAX) goto error;
      execute(op);
    }
  }

  // Compile mode
  else {
    switch (op) {
    case LEFT_BRACKET:
      compiling = false;
      break;
    case DOT_QUOTE:
      fvm.compile(FVM::OP_DOT_QUOTE);
      while (1) {
	while (!Serial.available());
	c = Serial.read();
	if (c == '\"') break;
	fvm.compile(c);
      }
      fvm.compile((FVM::code_t) 0);
      break;
    case LITERAL:
      fvm.literal(task.pop());
      break;
    case SEMICOLON:
      fvm.compile(FVM::OP_EXIT);
      compiling = false;
      break;
    default:
      if (op < SEMICOLON) {
	execute(op);
      }
      else if (!fvm.compile(op)) goto error;
    }
  }

  // Prompt on end of line
  if (c == '\n' && !compiling) {
    if (task.trace())
      Serial.println(F(" ok"));
    else
      fvm.execute(FVM::OP_DOT_S, task);
  }
  return;

 error:
  Serial.print(buffer);
  Serial.println(F(" ??"));
  compiling = false;
}

void execute(int op)
{
  if (fvm.execute(op, task) > 0)
    while (fvm.resume(task) > 0);
}
