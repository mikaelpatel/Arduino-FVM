/**
 * @file FVM/Compiler.ino
 * @version 1.1
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
 * Basic interactive compiler for the Forth Virtual Machine (FVM).
 * Compiles forth definitions, statements and generates virtual
 * machine code (C++).
 *
 * @section Words
 *
 * [ ( -- ) stop compile.
 * ] ( -- ) start compile.
 * literal ( n -- ) compile literal.
 *
 * ( comment ) start comment.
 * ." string" print string.
 * : ( -- ) start compile of function defintion.
 * ; ( -- ) end compile of function definition.
 * variable ( -- ) define variable.
 * constant ( value -- ) define constant with given value.
 *
 * compiled-words ( -- ) print list of compiled words.
 * generate-code ( -- ) print source code for compiled words.
 *
 * if ( -- addr ) start conditional block.
 * else ( addr1 -- addr ) end conditional block and start alternative.
 * then ( addr -- ) end conditional block.
 *
 * begin ( -- addr ) start iteration block.
 * again ( addr -- ) end infinite iteration block.
 * until ( addr -- ) end conditional iteration block.
 * while ( addr1 -- addr1 addr2 ) start conditional iteration block.
 * repeat ( addr1 addr2 -- ) end conditional iteration block.
 *
 * do ( -- addr ) start counting iteration block.
 * loop ( addr -- ) end counting iteration block.
 * +loop ( addr -- ) counting iteration block.
 *
 * mark> ( -- addr ) mark forward branch.
 * resolve> ( addr -- ) resolve forward branch.
 * <mark> ( -- addr ) mark backward branch.
 * <resolve> ( addr -- ) resolve backward branch.
 */

#include "FVM.h"

// Code generation dictionary word prefix (C++)
#define PREFIX "WORD"

/*
: mark> ( -- addr ) here 0 c, ;
: resolve> ( addr -- ) here over - swap c! ;
: <mark ( -- addr ) here ;
: <resolve ( addr -- ) here - c, ;
: if ( -- addr ) compile (0branch) mark> ; immediate
: then ( addr -- ) resolve> ; immediate
: else ( addr1 -- addr2 ) compile (branch) mark> swap resolve> ; immediate
: begin ( -- addr ) <mark ; immediate
: again ( addr -- ) compile (branch) <resolve ; immediate
: until ( addr -- ) compile (0branch) <resolve ; immediate
: while ( addr1 -- addr1 addr2 ) compile (0branch) mark> ; immediate
: repeat ( addr1 addr2 -- ) swap [compile] again resolve> ; immediate
: do ( -- addr1 addr2 ) compile (do) mark> <mark ; immediate
: loop ( addr1 addr2 -- ) compile (loop) <resolve resolve> ; immediate
: +loop ( addr1 addr2 -- ) compile (+loop) <resolve resolve> ; immediate
*/

FVM_COLON(0, FORWARD_MARK, "mark>")
  FVM_OP(HERE),
  FVM_OP(ZERO),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

FVM_COLON(1, FORWARD_RESOLVE, "resolve>")
  FVM_OP(HERE),
  FVM_OP(OVER),
  FVM_OP(MINUS),
  FVM_OP(SWAP),
  FVM_OP(C_STORE),
  FVM_OP(EXIT)
};

FVM_COLON(2, BACKWARD_MARK, "<mark")
  FVM_OP(HERE),
  FVM_OP(EXIT)
};

FVM_COLON(3, BACKWARD_RESOLVE, "<resolve")
  FVM_OP(HERE),
  FVM_OP(MINUS),
  FVM_OP(C_COMMA),
  FVM_OP(EXIT)
};

FVM_COLON(4, IF, "if")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
};

const int THEN = 5;
const char THEN_PSTR[] PROGMEM = "then";
#define THEN_CODE FORWARD_RESOLVE_CODE

FVM_COLON(6, ELSE, "else")
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(SWAP),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

const int BEGIN = 7;
const char BEGIN_PSTR[] PROGMEM = "begin";
#define BEGIN_CODE BACKWARD_MARK_CODE

FVM_COLON(8, AGAIN, "again")
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

FVM_COLON(9, UNTIL, "until")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
};

const int WHILE = 10;
const char WHILE_PSTR[] PROGMEM = "while";
#define WHILE_CODE IF_CODE

FVM_COLON(11, REPEAT, "repeat")
  FVM_OP(SWAP),
  FVM_CALL(AGAIN),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

FVM_COLON(12, DO, "do")
  FVM_OP(COMPILE),
  FVM_OP(DO),
  FVM_CALL(FORWARD_MARK),
  FVM_CALL(BACKWARD_MARK),
  FVM_OP(EXIT)
};

FVM_COLON(13, LOOP, "loop")
  FVM_OP(COMPILE),
  FVM_OP(LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

FVM_COLON(14, PLUS_LOOP, "+loop")
  FVM_OP(COMPILE),
  FVM_OP(PLUS_LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
};

FVM_SYMBOL(15, LEFT_BRACKET, "[");
FVM_SYMBOL(16, COMMENT, "(");
FVM_SYMBOL(17, DOT_QUOTE, ".\"");
FVM_SYMBOL(18, LITERAL, "literal");
FVM_SYMBOL(19, SEMICOLON, ";");
FVM_SYMBOL(20, RIGHT_BRACKET, "]");
FVM_SYMBOL(21, COLON, ":");
FVM_SYMBOL(22, VARIABLE, "variable");
FVM_SYMBOL(23, CONSTANT, "constant");
FVM_SYMBOL(24, COMPILED_WORDS, "compiled-words");
FVM_SYMBOL(25, GENERATE_CODE, "generate-code");

const FVM::code_P FVM::fntab[] PROGMEM = {
  (code_P) &FORWARD_MARK_CODE,
  (code_P) &FORWARD_RESOLVE_CODE,
  (code_P) &BACKWARD_MARK_CODE,
  (code_P) &BACKWARD_RESOLVE_CODE,
  (code_P) &IF_CODE,
  (code_P) &THEN_CODE,
  (code_P) &ELSE_CODE,
  (code_P) &BEGIN_CODE,
  (code_P) &AGAIN_CODE,
  (code_P) &UNTIL_CODE,
  (code_P) &WHILE_CODE,
  (code_P) &REPEAT_CODE,
  (code_P) &DO_CODE,
  (code_P) &LOOP_CODE,
  (code_P) &PLUS_LOOP_CODE
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
  (str_P) VARIABLE_PSTR,
  (str_P) CONSTANT_PSTR,
  (str_P) COMPILED_WORDS_PSTR,
  (str_P) GENERATE_CODE_PSTR,
  0
};

// Data area for the compiler code generation
#if defined(ARDUINO_ARCH_AVR)
const int DATA_MAX = 1024;
const int WORD_MAX = 32;
#else
const int DATA_MAX = 32 * 1024;
const int WORD_MAX = 128;
#endif
uint8_t data[DATA_MAX];

// Forth virtual machine and task
FVM fvm(data, DATA_MAX, WORD_MAX);
FVM::Task<64,32> task(Serial);
bool compiling = false;

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Compiler V1.1.0: started [Newline]"));
}

void loop()
{
  char buffer[32];
  int op;
  char c;

  // Scan and lookup word
  c = fvm.scan(buffer, task);
  op = fvm.lookup(buffer);

  // Check for literal value (word not found)
  if (op < 0) {
    char* endptr;
    int val = strtol(buffer, &endptr, task.m_base);
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
    else
      fvm.execute(op, task);
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
      fvm.scan(buffer, task);
      fvm.create(buffer);
      compiling = true;
      break;
    case VARIABLE:
      fvm.scan(buffer, task);
      fvm.variable(buffer);
      break;
    case CONSTANT:
      fvm.scan(buffer, task);
      fvm.constant(buffer, task.pop());
      break;
    case COMPILED_WORDS:
      compiled_words(Serial);
      break;
    case GENERATE_CODE:
      generate_code(Serial);
      fvm.forget(FVM::APPLICATION_MAX);
      break;
    default:
      goto error;
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
      if (op < SEMICOLON)
	fvm.execute(op, task);
      else if (!fvm.compile(op)) goto error;
    }
  }

  // Prompt on end of line
  if (c == '\n' && !compiling) Serial.println(F(" ok"));
  return;

 error:
  Serial.print(buffer);
  Serial.println(F(" ??"));
  compiling = false;
}

void compiled_words(Stream& ios)
{
  const char* s;
  int nr = 0;
  while ((s = fvm.name(nr)) != 0) {
    int len = ios.print(s);
    if (++nr % 5 == 0)
      ios.println();
    else {
      for (;len < 16; len++) ios.print(' ');
    }
  }
  ios.println();
}

void generate_code(Stream& ios)
{
  const char* name;
  uint8_t* dp;
  int val;

  // Generate function name strings and code
  for (int nr = 0; ((name = fvm.name(nr)) != 0); nr++) {
    ios.print(F("const char " PREFIX));
    ios.print(nr);
    ios.print(F("_PSTR[] PROGMEM = \""));
    ios.print(name);
    ios.println(F("\";"));
    dp = (uint8_t*) fvm.body(nr);
    switch (*dp) {
    case FVM::OP_VAR:
      ios.print(F("FVM::cell_t " PREFIX));
      ios.print(nr);
      ios.println(';');
      ios.print(F("const FVM::var_t " PREFIX));
      ios.print(nr);
      ios.println(F("_VAR[] PROGMEM = {"));
      ios.print(F("  FVM::OP_CONST, &" PREFIX));
      ios.println(nr);
      ios.println(F("};"));
      break;
    case FVM::OP_CONST:
      val = dp[2] << 8 | dp[1];
      ios.print(F("const FVM::const_t " PREFIX));
      ios.print(nr);
      ios.println(F("_CONST[] PROGMEM = {"));
      ios.print(F("  FVM::OP_CONST, "));
      ios.println(val);
      ios.println(F("};"));
      break;
    default:
      ios.print(F("const FVM::code_t " PREFIX));
      ios.print(nr);
      ios.print(F("_CODE[] PROGMEM = {\n  "));
      uint8_t* next = (uint8_t*) fvm.name(nr + 1);
      if (next == 0) next = fvm.dp();
      int length = next - dp;
      while (length) {
	int8_t code = (int8_t) *dp++;
	ios.print(code);
	if (--length) ios.print(F(", "));
      }
      ios.println();
      ios.println(F("};"));
    }
  }

  // Generate function code table
  ios.println(F("const FVM::code_P FVM::fntab[] PROGMEM = {"));
  for (int nr = 0; (dp = (uint8_t*) fvm.body(nr)) != 0; nr++) {
    ios.print(F("  (code_P) &" PREFIX));
    ios.print(nr);
    switch (*dp) {
    case FVM::OP_VAR:
      ios.print(F("_VAR"));
      break;
    case FVM::OP_CONST:
      ios.print(F("_CONST"));
      break;
    default:
      ios.print(F("_CODE"));
    }
    ios.println(',');
  }
  ios.println(F("  0"));
  ios.println(F("};"));

  // Generate function string table
  ios.println(F("const str_P FVM::fnstr[] PROGMEM = {"));
  for (int nr = 0; fvm.body(nr) != 0; nr++) {
    ios.print(F("  (str_P) " PREFIX));
    ios.print(nr++);
    ios.println(F("_PSTR,"));
  }
  ios.println(F("  0"));
  ios.println(F("};"));
}
