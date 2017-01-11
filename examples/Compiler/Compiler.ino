/**
 * @file FVM/Compiler.ino
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
 * Basic interactive compiler for the Forth Virtual Machine (FVM).
 * Compiles forth definitions, statements and generates virtual
 * machine code (C++).
 *
 * @section Words
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
 * room ( -- bytes ) number bytes left in data area.
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

// Compiler variants (performance/footprint)
#define USE_TAIL
#define USE_ALIAS

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
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_MARK)
#else
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
#endif
};

#if defined(USE_ALIAS)
const int THEN = 5;
const char THEN_PSTR[] PROGMEM = "then";
#define THEN_CODE FORWARD_RESOLVE_CODE
#else
FVM_COLON(5, THEN, "then")
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_RESOLVE)
#else
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};
#endif

FVM_COLON(6, ELSE, "else")
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
  FVM_CALL(FORWARD_MARK),
  FVM_OP(SWAP),
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_RESOLVE)
#else
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

#if defined(USE_ALIAS)
const int BEGIN = 7;
const char BEGIN_PSTR[] PROGMEM = "begin";
#define BEGIN_CODE BACKWARD_MARK_CODE
#else
FVM_COLON(7, BEGIN, "begin")
#if defined(USE_TAIL)
  FVM_TAIL(BACKWARD_MARK)
#else
  FVM_CALL(BACKWARD_MARK),
  FVM_OP(EXIT)
#endif
};
#endif

FVM_COLON(8, AGAIN, "again")
  FVM_OP(COMPILE),
  FVM_OP(BRANCH),
#if defined(USE_TAIL)
  FVM_TAIL(BACKWARD_RESOLVE)
#else
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

FVM_COLON(9, UNTIL, "until")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
#if defined(USE_TAIL)
  FVM_TAIL(BACKWARD_RESOLVE)
#else
  FVM_CALL(BACKWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

#if defined(USE_ALIAS)
const int WHILE = 10;
const char WHILE_PSTR[] PROGMEM = "while";
# define WHILE_CODE IF_CODE
#else
FVM_COLON(10, WHILE, "while")
  FVM_OP(COMPILE),
  FVM_OP(ZERO_BRANCH),
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_MARK)
#else
  FVM_CALL(FORWARD_MARK),
  FVM_OP(EXIT)
#endif
#endif

FVM_COLON(11, REPEAT, "repeat")
  FVM_OP(SWAP),
  FVM_CALL(AGAIN),
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_RESOLVE)
#else
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

FVM_COLON(12, DO, "do")
  FVM_OP(COMPILE),
  FVM_OP(DO),
  FVM_CALL(FORWARD_MARK),
#if defined(USE_TAIL)
  FVM_TAIL(BACKWARD_MARK)
#else
  FVM_CALL(BACKWARD_MARK),
  FVM_OP(EXIT)
#endif
};

FVM_COLON(13, LOOP, "loop")
  FVM_OP(COMPILE),
  FVM_OP(LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_RESOLVE)
#else
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

FVM_COLON(14, PLUS_LOOP, "+loop")
  FVM_OP(COMPILE),
  FVM_OP(PLUS_LOOP),
  FVM_CALL(BACKWARD_RESOLVE),
#if defined(USE_TAIL)
  FVM_TAIL(FORWARD_RESOLVE)
#else
  FVM_CALL(FORWARD_RESOLVE),
  FVM_OP(EXIT)
#endif
};

FVM_SYMBOL(15, COMMENT, "(");
FVM_SYMBOL(16, DOT_QUOTE, ".\"");
FVM_SYMBOL(17, SEMICOLON, ";");
FVM_SYMBOL(18, COLON, ":");
FVM_SYMBOL(19, VARIABLE, "variable");
FVM_SYMBOL(20, CONSTANT, "constant");
FVM_SYMBOL(21, COMPILED_WORDS, "compiled-words");
FVM_SYMBOL(22, GENERATE_CODE, "generate-code");
FVM_SYMBOL(23, ROOM, "room");

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
  (str_P) COMMENT_PSTR,
  (str_P) DOT_QUOTE_PSTR,
  (str_P) SEMICOLON_PSTR,
  (str_P) COLON_PSTR,
  (str_P) VARIABLE_PSTR,
  (str_P) CONSTANT_PSTR,
  (str_P) COMPILED_WORDS_PSTR,
  (str_P) GENERATE_CODE_PSTR,
  (str_P) ROOM_PSTR,
  0
};

// Data area for the shell
const int DATA_MAX = 1024;
uint8_t data[DATA_MAX];
bool compiling = false;
uint8_t* latest = data;

// Forth virtual machine and task
FVM fvm(data);
FVM::Task<64,32> task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Compiler: started [Newline]"));
}

void loop()
{
  char buffer[32];
  int op, val;
  char c;

  // Scan and lookup word
  c = fvm.scan(buffer, task);
  op = fvm.lookup(buffer);

  // Check for function call or literal value (word not found)
  if (op < 0) {
    if ((op = lookup(buffer)) < 0 && compiling) {
      fvm.compile(op);
    }
    else {
      char* endptr;
      int value = strtol(buffer, &endptr, task.m_base);
      if (*endptr != 0) {
	Serial.print(buffer);
	Serial.println(F(" ??"));
	fvm.dp(latest);
	compiling = false;
      }
      else {
	if (compiling) {
	  if (value < INT8_MIN || value > INT8_MAX) {
	    fvm.compile(FVM::OP_LIT);
	    fvm.compile(value);
	    fvm.compile(value >> 8);
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
  }

  // Check for kernel words
  else if (op < FVM::KERNEL_MAX) {
    if (compiling)
      fvm.compile(op);
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
    case COLON:
      fvm.scan(buffer, task);
      fvm.compile((FVM::code_t) 0);
      fvm.compile(buffer);
      compiling = true;
      break;
    case VARIABLE:
      fvm.scan(buffer, task);
      fvm.compile((FVM::code_t) 0);
      fvm.compile(buffer);
      fvm.compile(FVM::OP_VAR);
      fvm.compile((FVM::code_t) 0);
      fvm.compile((FVM::code_t) 0);
      *latest = fvm.dp() - latest;
      latest = fvm.dp();
      break;
    case CONSTANT:
      val = task.pop();
      fvm.scan(buffer, task);
      fvm.compile((FVM::code_t) 0);
      fvm.compile(buffer);
      fvm.compile(FVM::OP_CONST);
      fvm.compile(val);
      fvm.compile(val >> 8);
      *latest = fvm.dp() - latest;
      latest = fvm.dp();
      break;
    case ROOM:
      task.push(DATA_MAX - (fvm.dp() - data));
      break;
    case COMPILED_WORDS:
      words(Serial);
      break;
    case GENERATE_CODE:
      codegen(Serial);
      fvm.dp(data);
      latest = data;
      *latest = 0;
      break;
    default:
      Serial.print(buffer);
      Serial.println(F(" ??"));
    }
  }

  // Compile mode
  else {
    switch (op) {
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
    case SEMICOLON:
      fvm.compile(FVM::OP_EXIT);
      *latest = fvm.dp() - latest;
      latest = fvm.dp();
      compiling = false;
      break;
    default:
      if (op < SEMICOLON)
	fvm.execute(op, task);
      else {
	Serial.print(buffer);
	Serial.println(F(" ??"));
      }
    }
  }

  // Prompt on end of line
  if (c == '\n' && !compiling) Serial.println(F(" ok"));
}

int lookup(const char* s)
{
  int op = -1;
  uint8_t* dp = data;
  while (1) {
    if (!strcmp(s, (const char*) dp + 1)) return (op);
    uint8_t offset = *dp;
    if (offset == 0) return (0);
    dp += offset;
    op -= 1;
  }
}

void words(Stream& ios)
{
  uint8_t* dp = data;
  int nr = 0;
  while (dp < fvm.dp()) {
    ios.print(nr++);
    ios.print(':');
    ios.print(dp - data);
    ios.print(':');
    ios.print(*dp);
    ios.print(':');
    ios.println((const char*) dp + 1);
    dp += (uint8_t) *dp;
  }
}

void codegen(Stream& ios)
{
  int nr, last;
  uint8_t* dp;
  char* name;
  int val;

  // Generate function name strings and code
  dp = data;
  nr = 0;
  while (dp < fvm.dp()) {
    uint8_t length = *dp++ - 1;
    ios.print(F("const char " PREFIX));
    ios.print(nr);
    ios.print(F("_PSTR[] PROGMEM = \""));
    name = (char*) dp;
    ios.print(name);
    ios.println(F("\";"));
    uint8_t n = strlen(name) + 1;
    dp += n;
    switch (*dp) {
    case FVM::OP_VAR:
      ios.print(F("FVM::data_t " PREFIX));
      ios.print(nr);
      ios.println(';');
      ios.print(F("const FVM::var_t " PREFIX));
      ios.print(nr);
      ios.println(F("_VAR[] PROGMEM = {"));
      ios.print(F("  FVM::OP_VAR, &" PREFIX));
      ios.println(nr);
      ios.println(F("};"));
      dp += sizeof(FVM::var_t);
      break;
    case FVM::OP_CONST:
      val = dp[2] << 8 | dp[1];
      ios.print(F("const FVM::const_t " PREFIX));
      ios.print(nr);
      ios.println(F("_CONST[] PROGMEM = {"));
      ios.print(F("  FVM::OP_CONST, "));
      ios.println(val);
      ios.println(F("};"));
      dp += sizeof(FVM::const_t);
      break;
    default:
      ios.print(F("const FVM::code_t " PREFIX));
      ios.print(nr);
      ios.print(F("_CODE[] PROGMEM = {\n  "));
      while (n < length) {
	int8_t code = (int8_t) *dp++;
	ios.print(code);
	if (++n < length) ios.print(F(", "));
      }
      ios.println();
      ios.println(F("};"));
    }
    nr += 1;
  }
  last = nr;

  // Generate function code table
  nr = 0;
  dp = data;
  ios.println(F("const FVM::code_P FVM::fntab[] PROGMEM = {"));
  while (dp < fvm.dp()) {
    uint8_t length = *dp;
    name = (char*) dp + 1;
    uint8_t n = strlen(name) + 2;
    switch (dp[n]) {
    case FVM::OP_VAR:
      ios.print(F("  (code_P) &" PREFIX));
      ios.print(nr++);
      ios.print(F("_VAR"));
      break;
    case FVM::OP_CONST:
      ios.print(F("  (code_P) &" PREFIX));
      ios.print(nr++);
      ios.print(F("_CONST"));
      break;
    default:
      ios.print(F("  " PREFIX));
      ios.print(nr++);
      ios.print(F("_CODE"));
    }
    if (nr != last) ios.println(','); else ios.println();
    dp += length;
  }
  ios.println(F("};"));

  // Generate function string table
  nr = 0;
  dp = data;
  ios.println(F("const str_P FVM::fnstr[] PROGMEM = {"));
  while (dp < fvm.dp()) {
    uint8_t length = *dp;
    ios.print(F("  (str_P) " PREFIX));
    ios.print(nr++);
    ios.println(F("_PSTR,"));
    dp += length;
  }
  ios.println(F("  0"));
  ios.println(F("};"));
}
