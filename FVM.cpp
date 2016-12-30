/**
 * @file FVM.cpp
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
 */

#include "FVM.h"

// Forth Virtual Machine support macros
#define NEXT() goto INNER
#define OP(n) case OP_ ## n:
#define FETCH(ip) pgm_read_byte(ip)
#define WFETCH(ip) pgm_read_word(ip)
#define CALL(fn) tp = fn; goto FNCALL
#define FNSTR(ir) (const __FlashStringHelper*) pgm_read_word(&fnstr[-ir-1])
#define OPSTR(ir) (const __FlashStringHelper*) pgm_read_word(&opstr[ir])

int FVM::lookup(const char* name)
{
  const char* s;

  // Search kernel dictionary, return index
  for (int i = 0; (s = (const char*) pgm_read_word(&opstr[i])) != 0; i++)
    if (!strcmp_P(name, s)) return (i);

  // Search sketch dictionary, return adjusted index
  for (int i = 0; (s = (const char*) pgm_read_word(&fnstr[i])) != 0; i++)
    if (!strcmp_P(name, s)) return (i+128);

  // Return error code
  return (-1);
}

int FVM::resume(task_t& task)
{
  // Restore virtual machine state
  Stream& ios = task.m_ios;
  data_t* sp = task.m_sp;
  data_t tos = *sp--;
  const code_t* ip = task.m_ip;
  const code_t** rp = task.m_rp;
  const code_t* tp;
  data_t tmp;
  int8_t ir;

  // Benchmark support in trace mode; measure micro-seconds per operation
#if defined(FVM_TRACE)
  uint32_t start = micros();
#endif

 INNER:
  // Positive opcode (0..127) are direct operation codes. Negative
  // opcodes (-1..-128) are negative index (plus one) in function
  // table. Direct operation codes may be implemented as a primitive
  // or as an internal function call.
#if !defined(FVM_TRACE)

  while ((ir = (int8_t) FETCH(ip++)) < 0) {
    *++rp = ip;
    ip = (code_P) pgm_read_word(fntab-ir-1);
  }

#else
  // Trace execution; micro-seconds, instruction pointer,
  // instruction/function, and stack contents
  do {
    if (task.m_trace) {
      // Print measurement of latest operation; micro-seconds
      ios.print(micros() - start);
      ios.print(':');
      // Print current instruction pointer
      ios.print((uint16_t) ip);
      ios.print(':');
    }
    // Fetch next instruction and check for function call or primitive
    // Print name of function or primitive
    ir = (int8_t) FETCH(ip++);
    if (ir < 0 ) {
      *++rp = ip;
      ip = (code_P) pgm_read_word(fntab-ir-1);
      if (task.m_trace) {
	ios.print(127-ir);
	ios.print(':');
	ios.print(FNSTR(ir));
      }
    }
    else if (task.m_trace) {
      ios.print(ir);
#if defined(FVM_DICT)
      ios.print(':');
      ios.print(OPSTR(ir));
#endif
    }
    // Print stack contents
    if (task.m_trace) {
      tmp = (sp - task.m_sp0);
      ios.print(F(":["));
      ios.print(tmp);
      ios.print(F("]: "));
      if (tmp > 0) {
	data_t* tp = task.m_sp0 + 1;
	while (--tmp) {
	  ios.print(*++tp);
	  ios.print(' ');
	}
	ios.print(tos);
      }
      ios.println();
    }
  } while (ir < 0);
  // Flush output and start measurement
  ios.flush();
  start = micros();
#endif

  // Dispatch instruction; primitive or internal function call
 DISPATCH:
  switch (ir) {

  // exit ( rp: ip -- )
  // Exit from call. Pop instruction pointer from return stack
  OP(EXIT)
    ip = *rp--;
  NEXT();

  // (literal) ( -- x )
  // Push literal data (big-endian)
  OP(LITERAL)
    *++sp = tos;
    tos = (int8_t) FETCH(ip++);
    tos = ((tos << 8) | FETCH(ip++));
  NEXT();

  // (cliteral) ( -- x )
  // Push literal data
  OP(C_LITERAL)
    *++sp = tos;
    tos = (int8_t) FETCH(ip++);
  NEXT();

  // (var) ( -- addr )
  // Push address of variable
  OP(VAR)

  // (const) ( -- addr )
  // Push value of contant
  OP(CONST)
    *++sp = tos;
    tos = (data_t) WFETCH(ip);
    ip = *rp--;
  NEXT();

  // (does) ( -- value )
  // Push literal constant and return
  OP(DOES)
    *++sp = tos;
    tp = *rp--;
    tos = (data_t) WFETCH(tp);
  NEXT();

  // (param) ( x0..xn -- x0..xn x0 )
  // Duplicate inline index stack element to top of stack
  OP(PARAM)
    *++sp = tos;
    ir = (int8_t) FETCH(ip++);
    tos = *(sp - ir);
  NEXT();

  // (sliteral) ( -- addr )
  // Push pointer to literal and branch
  OP(S_LITERAL)
    *++sp = tos;
    tos = (data_t) ip + 1;

  // (branch) ( -- )
  // Branch always (8-bit offset, -128..127)
  OP(BRANCH)
    ir = (int8_t) FETCH(ip++);
    ip += ir;
  NEXT();

  // (0branch) ( flag -- )
  // Branch zero equal/false (8-bit offset, -128..127)
  OP(ZERO_BRANCH)
    ir = (int8_t) FETCH(ip++);
    if (tos == 0) ip += ir;
    tos = *sp--;
  NEXT();

  // execute ( n -- )
  // Execute primitive or function (as returned by lookup)
  OP(EXECUTE)
    if (tos < 128) {
      ir = tos;
      tos = *sp--;
      goto DISPATCH;
    }
    else {
      *++rp = ip;
      ip = (code_P) pgm_read_word(fntab+(tos-128));
      tos = *sp--;
    }
  NEXT();

  // trace ( x -- )
  // Set trace mode.
  OP(TRACE)
    task.m_trace = tos;
    tos = *sp--;
  NEXT();

  // c@ ( addr -- x )
  // Load character given address on top of stack
  OP(C_FETCH)
    tos = *((int8_t*) tos);
  NEXT();

  // c! ( x addr -- )
  // Store character given address on top of stack
  OP(C_STORE)
    *((int8_t*) tos) = *sp--;
    tos = *sp--;
  NEXT();

  // @ ( addr -- x )
  // Load data given address on top of stack
  OP(FETCH)
    tos = *((data_t*) tos);
  NEXT();

  // ! ( x addr -- )
  // Store data given address on top of stack
  OP(STORE)
    *((data_t*) tos) = *sp--;
    tos = *sp--;
  NEXT();

  // +! ( n addr -- )
  // Add data given address on top of stack
  OP(PLUS_STORE)
#if 0
    *((data_t*) tos) += *sp--;
    tos = *sp--;
  NEXT();
#else
  // : +! ( n addr -- ) dup >r @ + r> ! ;
  static const code_t PLUS_STORE_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(TO_R),
    FVM_OP(FETCH),
    FVM_OP(PLUS),
    FVM_OP(R_FROM),
    FVM_OP(STORE),
    FVM_OP(EXIT)
  };
  CALL(PLUS_STORE_CODE);
#endif

  // dp ( -- addr )
  // Push address to data pointer
  OP(DP)
    *++sp = tos;
    tos = (data_t) &m_dp;
  NEXT();

  // here ( -- addr )
  // Push data pointer
  OP(HERE)
#if 0
    *++sp = tos;
    tos = (data_t) m_dp;
  NEXT();
#else
  // : here ( -- addr ) dp @ ;
  static const code_t HERE_CODE[] PROGMEM = {
    FVM_OP(DP),
    FVM_OP(FETCH),
    FVM_OP(EXIT)
  };
  CALL(HERE_CODE);
#endif

  // allot ( n -- )
  // Allocate number of bytes from data area
  OP(ALLOT)
#if 0
    m_dp += tos;
    tos = *sp--;
  NEXT();
#else
  // : allot ( n -- ) dp +! ;
  static const code_t ALLOT_CODE[] PROGMEM = {
    FVM_OP(DP),
    FVM_OP(PLUS_STORE),
    FVM_OP(EXIT)
  };
  CALL(ALLOT_CODE);
#endif

  // , ( x -- )
  // Allocate and assign from top of stack
  OP(COMMA)
#if 0
    *((data_t*) task.m_dp) = tos;
    task.m_dp += sizeof(data_t);
    tos = *sp--;
  NEXT();
#else
  // : , ( x -- ) here ! cell allot ;
  static const code_t COMMA_CODE[] PROGMEM = {
    FVM_OP(HERE),
    FVM_OP(STORE),
    FVM_OP(CELL),
    FVM_OP(ALLOT),
    FVM_OP(EXIT)
  };
  CALL(COMMA_CODE);
#endif

  // c, ( x -- )
  // Allocate and assign character from top of stack
  OP(C_COMMA)
#if 0
    *task.m_dp++ = tos;
    tos = *sp--;
  NEXT();
#else
  // : c, ( x -- ) here c! 1 allot ;
  static const code_t C_COMMA_CODE[] PROGMEM = {
    FVM_OP(HERE),
    FVM_OP(C_STORE),
    FVM_OP(ONE),
    FVM_OP(ALLOT),
    FVM_OP(EXIT)
  };
  CALL(C_COMMA_CODE);
#endif

  // cells ( x -- y )
  // Convert cells to bytes for allot
  OP(CELLS)
#if 1
    tos *= sizeof(data_t);
  NEXT();
#else
  // : cells ( x -- y ) cell * ;
  static const code_t CELLS_CODE[] PROGMEM = {
    FVM_OP(CELL),
    FVM_OP(STAR),
    FVM_OP(EXIT)
  };
  CALL(CELLS_CODE);
#endif

  // >r ( x -- rp: x )
  // Push data to return stack
  OP(TO_R)
    *++rp = (code_t*) tos;
    tos = *sp--;
  NEXT();

  // r> ( rp: x -- x )
  // Pop data from return stack
  OP(R_FROM)
    *++sp = tos;
    tos = (data_t) *rp--;
  NEXT();

  // r@ ( rp: x -- x, rp: x )
  // Copy data from top of return stack
  OP(R_FETCH)
    *++sp = tos;
    tos = (data_t) *rp;
  NEXT();

  // ?r ( rp: x -- x-1/0, rp: 1 -- /-1 )
  // Decrement top of return stack and check zero
  OP(QR)
    *++sp = tos;
    tmp = ((data_t) *rp) - 1;
    if (tmp != 0) {
      *rp = (code_t*) tmp;
      tos = 0;
    }
    else {
      rp -= 1;
      tos = -1;
    }
  NEXT();

  // sp ( -- addr )
  // Push stack pointer
  OP(SP)
    *++sp = tos;
    tos = (data_t) sp;
  NEXT();

  // depth ( x1..xn -- x1..xn n )
  // Push depth of data stack
  OP(DEPTH)
    tmp = (sp - task.m_sp0);
    *++sp = tos;
    tos = tmp;
  NEXT();

  // drop ( x -- )
  // Drop top of stack
  OP(DROP)
    tos = *sp--;
  NEXT();

  // nip ( x y -- y )
  // Drop next of stack
  OP(NIP)
#if 1
    sp -= 1;
  NEXT();
#else
  // : nip ( x y -- y ) swap drop ;
  static const code_t NIP_CODE[] PROGMEM = {
    FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(NIP_CODE);
#endif

  // empty ( xn...x0 -- )
  // Empty data stack
  OP(EMPTY)
    sp = task.m_sp0;
  NEXT();

  // dup ( x -- x x )
  // Duplicate top of stack
  OP(DUP)
#if 1
    *++sp = tos;
  NEXT();
#else
  // : dup ( x x -- x x ) param: 0  ;
  static const code_t DUP_CODE[] PROGMEM = {
    FVM_OP(PARAM), 0,
    FVM_OP(EXIT)
  };
  CALL(DUP_CODE);
#endif

  // ?dup ( x -- x x | 0 -- 0 )
  // Duplicate non zero top of stack
  OP(QDUP)
#if 0
    if (tos != 0) *++sp = tos;
  NEXT();
#else
  // : ?dup ( x -- x x | 0 -- 0 ) dup if dup then ;
  static const code_t QDUP_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(ZERO_BRANCH), 1,
      FVM_OP(DUP),
    FVM_OP(EXIT)
  };
  CALL(QDUP_CODE);
#endif

  // over ( x y -- x y x )
  // Duplicate next top of stack
  OP(OVER)
#if 1
    tmp = *sp;
    *++sp = tos;
    tos = tmp;
  NEXT();
#else
  // : over ( x y -- x y x ) param: 1 ;
  static const code_t OVER_CODE[] PROGMEM = {
    FVM_OP(PARAM), 1,
    FVM_OP(EXIT)
  };
  CALL(OVER_CODE);
#endif

  // tuck ( x y -- y x y )
  // Duplicate top of stack and rotate
  OP(TUCK)
#if 0
    tmp = *sp;
    *sp = tos;
    *++sp = tmp;
  NEXT();
#else
  // : tuck ( x y -- y x y ) swap over ;
  static const code_t TUCK_CODE[] PROGMEM = {
    FVM_OP(SWAP),
    FVM_OP(OVER),
    FVM_OP(EXIT)
  };
  CALL(TUCK_CODE);
#endif

  // pick ( x0..xn n -- x0..xn x0 )
  // Duplicate index stack element to top of stack
  OP(PICK)
    tos = *(sp - tos);
  NEXT();

  // swap ( x y -- y x )
  // Swap top two stack elements
  OP(SWAP)
#if 1
    tmp = tos;
    tos = *sp;
    *sp = tmp;
  NEXT();
#else
  // : swap ( x y -- y x ) 1 roll ;
  static const code_t SWAP_CODE[] PROGMEM = {
    FVM_OP(ONE),
    FVM_OP(ROLL),
    FVM_OP(EXIT)
  };
  CALL(SWAP_CODE);
#endif

  // rot ( x y z -- y z x )
  // Rotate up top three stack elements
  OP(ROT)
#if 1
    tmp = tos;
    tos = *(sp - 1);
    *(sp - 1) = *sp;
    *sp = tmp;
  NEXT();
#else
  // : rot ( x y z -- z y x ) 2 roll ;
  static const code_t ROT_CODE[] PROGMEM = {
    FVM_OP(TWO),
    FVM_OP(ROLL),
    FVM_OP(EXIT)
  };
  CALL(ROT_CODE);
#endif

  // -rot ( x y z -- z x y )
  // Rotate down top three stack elements
  OP(MINUS_ROT)
#if 0
    tmp = tos;
    tos = *sp;
    *sp = *(sp - 1);
    *(sp - 1) = tmp;
  NEXT();
#else
  // : -rot ( x y z -- z x y ) rot rot ;
  static const code_t MINUS_ROT_CODE[] PROGMEM = {
    FVM_OP(ROT),
    FVM_OP(ROT),
    FVM_OP(EXIT)
  };
  CALL(MINUS_ROT_CODE);
#endif

  // roll ( x0..xn n -- x1..xn x0 )
  // Rotate up stack elements
  OP(ROLL)
    tmp = tos;
    tos = sp[-tmp];
    for (; tmp > 0; tmp--)
      sp[-tmp] = sp[-tmp + 1];
    sp -= 1;
  NEXT();

  // : 2swap ( x1 x2 y1 y2 -- y1 y2 x1 x2 ) rot >r rot r> ;
  // Swap two double stack elements
  OP(TWO_SWAP)
  static const code_t TWO_SWAP_CODE[] PROGMEM = {
    FVM_OP(ROT),
    FVM_OP(TO_R),
    FVM_OP(ROT),
    FVM_OP(R_FROM),
    FVM_OP(EXIT)
  };
  CALL(TWO_SWAP_CODE);

  // : 2dup ( x1 x2 -- x1 x2 x1 x2) over over ;
  // Duplicate double stack elements
  OP(TWO_DUP)
  static const code_t TWO_DUP_CODE[] PROGMEM = {
    FVM_OP(OVER),
    FVM_OP(OVER),
    FVM_OP(EXIT)
  };
  CALL(TWO_DUP_CODE);

  // : 2over ( x1 x2 y1 y2 -- x1 y1 y1 y2 x1 x2 ) param: 3 param: 3 ;
  // Duplicate double next top of stack
  OP(TWO_OVER)
  static const code_t TWO_OVER_CODE[] PROGMEM = {
    FVM_OP(PARAM), 3,
    FVM_OP(PARAM), 3,
    FVM_OP(EXIT)
  };
  CALL(TWO_OVER_CODE);

  // : 2drop ( x1 x2 -- ) drop drop ;
  // Drop double top of stack
  OP(TWO_DROP)
  static const code_t TWO_DROP_CODE[] PROGMEM = {
    FVM_OP(DROP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(TWO_DROP_CODE);

  // cell ( -- n )
  // Size of data element in bytes
  OP(CELL)
    *++sp = tos;
    tos = sizeof(data_t);
  NEXT();

  // -2 ( -- -2 )
  // Constant -2
  OP(MINUS_TWO)
    *++sp = tos;
    tos = -2;
  NEXT();

  // -1 ( -- -1 )
  // Constant -1
  OP(MINUS_ONE)

  // TRUE ( -- -1 )
  // Constant true (alias -1)
  OP(TRUE)
    *++sp = tos;
    tos = -1;
  NEXT();

  // 0 ( -- 0 )
  // Constant 0
  OP(ZERO)

  // FALSE ( -- 0 )
  // Constant false
  OP(FALSE)
    *++sp = tos;
    tos = 0;
  NEXT();

  // 1 ( -- 1 )
  // Constant 1
  OP(ONE)
    *++sp = tos;
    tos = 1;
  NEXT();

  // 2 ( -- 2 )
  // Constant 2
  OP(TWO)
    *++sp = tos;
    tos = 2;
  NEXT();

  // invert ( x -- ~x )
  // Bitwise complement top of stack
  OP(INVERT)
    tos = ~tos;
  NEXT();

  // and ( x y -- x&y )
  // Bitwise AND top of stack
  OP(AND)
    tos = *sp-- & tos;
  NEXT();

  // or ( x y -- x|y )
  // Bitwise OR top of stack
  OP(OR)
    tos = *sp-- | tos;
  NEXT();

  // xor ( x y -- x^y )
  // Bitwise XOR top of stack
  OP(XOR)
    tos = *sp-- ^ tos;
  NEXT();

  // negate ( x -- -x )
  // Negate top of stack
  OP(NEGATE)
#if 1
    tos = -tos;
  NEXT();
#else
  // : negate ( x -- -x) invert 1+ ;
  static const code_t NEGATE_CODE[] PROGMEM = {
    FVM_OP(INVERT),
    FVM_OP(ONE_PLUS),
    FVM_OP(EXIT)
  };
  CALL(NEGATE_CODE);
#endif

  // 1+ ( x -- x+1 )
  // Increment top of stack
  OP(ONE_PLUS)
    tos += 1;
  NEXT();

  // 1- ( x -- x-1 )
  // Decrement top of stack
  OP(ONE_MINUS)
    tos -= 1;
  NEXT();

  // 2+ ( x -- x+2 )
  // Increment top of stack by two
  OP(TWO_PLUS)
    tos += 2;
  NEXT();

  // 2- ( x -- x-2 )
  // Decrement top of stack by two
  OP(TWO_MINUS)
    tos -= 2;
  NEXT();

  // 2* ( x -- x*2 )
  // Multiply top of stack by two
  OP(TWO_STAR)
    tos <<= 1;
  NEXT();

  // 2/ ( x -- x*2 )
  // Divide top of stack by two
  OP(TWO_SLASH)
    tos >>= 1;
  NEXT();

  // + ( x y -- x+y )
  // Add two top of stack elements
  OP(PLUS)
    tos = *sp-- + tos;
  NEXT();

  // - ( x y -- x-y )
  // Subtract two top of stack elements
  OP(MINUS)
    tos = *sp-- - tos;
  NEXT();

  // * ( x y -- x*y )
  // Multiply two top of stack elements
  OP(STAR)
    tos = *sp-- * tos;
  NEXT();

  // */ ( x y z -- x*y/z )
  // Multiply and divide top of stack elements
  OP(STAR_SLASH)
    tmp = *sp--;
    tos = (((data2_t) tmp) * (*sp--)) / tos;
  NEXT();

  // / ( x y -- x/y )
  // Divide two top of stack elements
  OP(SLASH)
    tos = *sp-- / tos;
  NEXT();

  // mod ( x y -- x%y )
  // Remainder after division of two top of stack elements
  OP(MOD)
    tos = *sp-- % tos;
  NEXT();

  // /mod ( x y -- x/y x%y )
  // Divide two top of stack elements
  OP(SLASH_MOD)
    tmp = *sp / tos;
    tos = *sp % tos;
    *sp = tmp;
  NEXT();

  // lshift ( x n -- x<<n )
  // Logical shift left given number of positions
  OP(LSHIFT)
    tos = *sp-- << tos;
  NEXT();

  // rshift ( x n -- x>>n )
  // Logical shift right given number of positions
  OP(RSHIFT)
    tos = *sp-- >> tos;
  NEXT();

  // within ( x low high -- flag )
  // Check if value is within boundary
  OP(WITHIN)
#if 0
    tmp = *sp--;
    tos = ((*sp <= tos) & (*sp >= tmp)) ? -1 : 0;
    sp--;
  NEXT();
#else
  // : within ( x low high -- flag ) >r over swap < swap r> > or not ;
  static const code_t WITHIN_CODE[] PROGMEM = {
    FVM_OP(TO_R),
    FVM_OP(OVER),
    FVM_OP(SWAP),
    FVM_OP(LESS),
    FVM_OP(SWAP),
    FVM_OP(R_FROM),
    FVM_OP(GREATER),
    FVM_OP(OR),
    FVM_OP(NOT),
    FVM_OP(EXIT)
  };
  CALL(WITHIN_CODE);
#endif

  // abs ( x -- |x| )
  // Absolute value of top of stack
  OP(ABS)
#if 1
    if (tos < 0) tos = -tos;
  NEXT();
#else
  // : abs ( x -- |x| ) dup 0< if negate then ;
  static const code_t ABS_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(ZERO_LESS),
    FVM_OP(ZERO_BRANCH), 1,
      FVM_OP(NEGATE),
    FVM_OP(EXIT)
  };
  CALL(ABS_CODE);
#endif

  // min ( x y -- min(x,y) )
  // Minimum value of top two stack elements
  OP(MIN)
#if 0
    tmp = *sp--;
    if (tmp < tos) tos = tmp;
  NEXT();
#else
  // : min ( x y -- min(x,y)) 2dup > if swap then drop ;
  static const code_t MIN_CODE[] PROGMEM = {
    FVM_OP(TWO_DUP),
    FVM_OP(GREATER),
    FVM_OP(ZERO_BRANCH), 1,
      FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(MIN_CODE);
#endif

  // max ( x y -- max(x,y))
  // Maximum value of top two stack elements
  OP(MAX)
#if 0
    tmp = *sp--;
    if (tmp > tos) tos = tmp;
  NEXT();
#else
  // : max ( x y -- max(x,y)) 2dup < if swap then drop ;
  static const code_t MAX_CODE[] PROGMEM = {
    FVM_OP(TWO_DUP),
    FVM_OP(LESS),
    FVM_OP(ZERO_BRANCH), 1,
      FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(MAX_CODE);
#endif

  // bool ( x<>0: x -- TRUE, else FALSE )
  // Convert top of stack to boolean (alias 0<>)
  OP(BOOL)

  // 0<> ( x<>0: x -- -1, else 0 )
  // Top of stack not equal zero
  OP(ZERO_NOT_EQUALS)
#if 1
    tos = (tos != 0) ? -1 : 0;
  NEXT();
#else
  // : 0<> ( x<>0: x -- -1, else 0 ) 0= not ;
  static const code_t ZERO_NOT_EQUALS_CODE[] PROGMEM = {
    FVM_OP(ZERO_EQUALS),
    FVM_OP(NOT),
    FVM_OP(EXIT)
  };
  CALL(ZERO_NOT_EQUALS_CODE);
#endif

  // 0< ( x<0: x -- -1, else 0 )
  // Top of stack less than zero
  OP(ZERO_LESS)
    tos = (tos < 0) ? -1 : 0;
  NEXT();

  // not ( x==0: x -- -1, else 0 )
  // Convert top of stack to invert boolean (alias 0=)
  OP(NOT)

  // 0= ( x==0: x -- -1, else 0 )
  // Top of stack less equal zero
  OP(ZERO_EQUALS)
    tos = (tos == 0) ? -1 : 0;
  NEXT();

  // 0> ( x>0: x -- -1, else 0 )
  // Top of stack less greater than zero
  OP(ZERO_GREATER)
    tos = (tos > 0) ? -1 : 0;
  NEXT();

  // <> ( x<>y: x y -- -1, else 0 )
  // Top two stack elements are not equal
  OP(NOT_EQUALS)
#if 0
    tos = (*sp-- != tos) ? -1 : 0;
  NEXT();
#else
  // : <> ( x<>y: x y -- -1, else 0 ) - bool ;
  static const code_t NOT_EQUALS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(BOOL),
    FVM_OP(EXIT)
  };
  CALL(NOT_EQUALS_CODE);
#endif

  // < ( x<y: x y -- -1, else 0 )
  // Second element is less than top element
  OP(LESS)
#if 0
    tos = (*sp-- < tos) ? -1 : 0;
  NEXT();
#else
  // : < ( x<y: x y -- -1, else 0 ) - 0< ;
  static const code_t LESS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_LESS),
    FVM_OP(EXIT)
  };
  CALL(LESS_CODE);
#endif

  // = ( x==y: x y -- -1, else 0 )
  // Top two stack elements are equal
  OP(EQUALS)
#if 0
    tos = (*sp-- == tos) ? -1 : 0;
  NEXT();
#else
  // : = ( x==y: x y -- -1, else 0 ) - 0= ;
  static const code_t EQUALS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_EQUALS),
    FVM_OP(EXIT)
  };
  CALL(EQUALS_CODE);
#endif

  // > ( x>y: x y -- -1, else 0 )
  // Second element is greater than top element
  OP(GREATER)
#if 0
    tos = (*sp-- > tos) ? -1 : 0;
  NEXT();
#else
  // : > ( x>y: x y -- -1, else 0 ) - 0> ;
  static const code_t GREATER_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_GREATER),
    FVM_OP(EXIT)
  };
  CALL(GREATER_CODE);
#endif

  // u< ( x<y: x y -- -1, else 0 )
  // Second element is unsigned less than top element
  OP(U_LESS)
    tos = ((udata_t) *sp-- < (udata_t) tos) ? -1 : 0;
  NEXT();

  // lookup ( str -- n )
  // Lookup string in dictionary
  OP(LOOKUP)
    tos = lookup((const char*) tos);
  NEXT();

  // >body ( n -- addr )
  // Access data area for given token (must be greater than 127).
  OP(TO_BODY)
    tp = (code_P) pgm_read_word(fntab+(tos-128));
    tos = (data_t) WFETCH(tp+1);
  NEXT();

  // words ( -- )
  // Print list of operations/functions
  OP(WORDS)
#if 0
  {
    const char* s;
    int len;
    int j = 0;
    for (int i = 0; (s = (const char*) pgm_read_word(&opstr[i])) != 0; i++) {
      ios.print((const __FlashStringHelper*) s);
      if (++j % 5 == 0)
	ios.println();
      else {
	len = 16 - strlen_P(s);
	while (len-- > 0) ios.print(' ');
      }
    }
    for (int i = 0; (s = (const char*) pgm_read_word(&fnstr[i])) != 0; i++) {
      ios.print((const __FlashStringHelper*) s);
      if (++j % 5 == 0)
	ios.println();
      else {
	len = 16 - strlen_P(s);
	while (len-- > 0) ios.print(' ');
      }
    }
  }
  NEXT();
#else
  // NOTE: Forth implementation only lists words in kernel dictionary.
  // Application words are currently not listed.
  // : words ( -- )
  //   0
  //   begin
  //     dup .name ?dup
  //   while
  //     16 swap - spaces
  //     1+ dup 5 mod not
  //     if cr then
  //   repeat
  //   cr drop ;
  static const code_t WORDS_CODE[] PROGMEM = {
    FVM_OP(ZERO),
      FVM_OP(DUP),
      FVM_OP(DOT_NAME),
      FVM_OP(QDUP),
    FVM_OP(ZERO_BRANCH), 16,
      FVM_CLIT(16),
      FVM_OP(SWAP),
      FVM_OP(MINUS),
      FVM_OP(SPACES),
      FVM_OP(ONE_PLUS),
      FVM_OP(DUP),
      FVM_CLIT(5),
      FVM_OP(MOD),
      FVM_OP(NOT),
      FVM_OP(ZERO_BRANCH), -18,
        FVM_OP(CR),
    FVM_OP(BRANCH), -21,
    FVM_OP(CR),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(WORDS_CODE);
#endif

  // base ( -- addr )
  // Number conversion base.
  OP(BASE)
    *++sp = tos;
    tos = (data_t) &task.m_base;
  NEXT();

  // hex ( -- )
  // Set hexa-decimal number conversion base.
  OP(HEX)
#if 0
    task.m_base = 16;
  NEXT();
#else
  // : hex ( -- ) 16 base ! ;
  static const code_t HEX_CODE[] PROGMEM = {
    FVM_CLIT(16),
    FVM_OP(BASE),
    FVM_OP(STORE),
    FVM_OP(EXIT)
  };
  CALL(HEX_CODE);
#endif

  // decimal ( -- )
  // Set decimal number conversion base.
  OP(DECIMAL)
#if 0
    task.m_base = 10;
  NEXT();
#else
  // : decimal ( -- ) 10 base ! ;
  static const code_t DECIMAL_CODE[] PROGMEM = {
    FVM_CLIT(10),
    FVM_OP(BASE),
    FVM_OP(STORE),
    FVM_OP(EXIT)
  };
  CALL(DECIMAL_CODE);
#endif

  // ?key ( -- c -1/0 )
  // Read character if available
  OP(QKEY)
    *++sp = tos;
    if (ios.available()) {
      *++sp = ios.read();
      tos = -1;
    }
    else {
      tos = 0;
    }
  NEXT();

  // key ( -- c ) begin ?key while yield repeat ;
  // Wait for character and read
  OP(KEY)
  static const code_t KEY_CODE[] PROGMEM = {
      FVM_OP(QKEY),
      FVM_OP(NOT),
    FVM_OP(ZERO_BRANCH), 3,
      FVM_OP(YIELD),
    FVM_OP(BRANCH), -7,
    FVM_OP(EXIT)
  };
  CALL(KEY_CODE);

  // emit ( c -- )
  // Print character
  OP(EMIT)
    ios.print((char) tos);
    tos = *sp--;
  NEXT();

  // cr ( -- )
  // Print new-line
  OP(CR)
#if 1
    ios.println();
  NEXT();
#else
  // : cr ( -- ) '\n' emit ;
  static const code_t CR_CODE[] PROGMEM = {
    FVM_CLIT('\n'),
    FVM_OP(EMIT),
    FVM_OP(EXIT)
  };
  CALL(CR_CODE);
#endif

  // space ( -- )
  // Print space
  OP(SPACE)
#if 1
    ios.print(' ');
  NEXT();
#else
  // : space ( -- ) ' ' emit ;
  static const code_t SPACE_CODE[] PROGMEM = {
    FVM_CLIT(' '),
    FVM_OP(EMIT),
    FVM_OP(EXIT)
  };
  CALL(SPACE_CODE);
#endif

  // spaces ( n -- )
  // Print spaces
  OP(SPACES)
#if 0
    while (tos--) ios.print(' ');
    tos = *sp--;
  NEXT();
#else
  // : spaces ( n -- ) begin ?dup while space 1- repeat ;
  static const code_t SPACES_CODE[] PROGMEM = {
      FVM_OP(QDUP),
    FVM_OP(ZERO_BRANCH), 4,
      FVM_OP(SPACE),
      FVM_OP(ONE_MINUS),
    FVM_OP(BRANCH), -7,
    FVM_OP(EXIT)
  };
  CALL(SPACES_CODE);
#endif

  // u. ( ux -- )
  // Print value on top of stack as unsigned number.
  OP(U_DOT)
    ios.print((udata_t) tos, task.m_base);
    tos = *sp--;
  NEXT();

  // . ( x -- )
  // Print value on top of stack as signed number.
  OP(DOT)
#if 0
    ios.print(tos, task.m_base);
    ios.print(' ');
    tos = *sp--;
  NEXT();
#else
  // : . ( n -- )
  // base @ 10 = if dup 0< if '-' emit negate then then u. space ;
  static const code_t DOT_CODE[] PROGMEM = {
    FVM_OP(BASE),
    FVM_OP(FETCH),
    FVM_CLIT(10),
    FVM_OP(EQUALS),
    FVM_OP(ZERO_BRANCH), 8,
      FVM_OP(DUP),
      FVM_OP(ZERO_LESS),
      FVM_OP(ZERO_BRANCH), 4,
        FVM_CLIT('-'),
        FVM_OP(EMIT),
        FVM_OP(NEGATE),
    FVM_OP(U_DOT),
    FVM_OP(SPACE),
    FVM_OP(EXIT)
  };
  CALL(DOT_CODE);
#endif

  // .s ( -- )
  // Print stack contents
  OP(DOT_S)
#if 0
    tmp = (sp - task.m_sp0);
    ios.print('[');
    ios.print(tmp);
    ios.print(F("]: "));
    if (tmp > 0) {
      data_t* tp = task.m_sp0 + 1;
      while (--tmp) {
	ios.print(*++tp, task.m_base);
	ios.print(' ');
      }
      ios.print(tos);
    }
    ios.println();
  NEXT();
#else
  // : .s ( -- )
  // depth dup '[' emit u. ']' emit ':' emit space
  // begin ?dup while dup pick . 1- repeat
  // cr ;
  static const code_t DOT_S_CODE[] PROGMEM = {
    FVM_OP(DEPTH),
    FVM_OP(DUP),
    FVM_CLIT('['),
    FVM_OP(EMIT),
    FVM_OP(U_DOT),
    FVM_CLIT(']'),
    FVM_OP(EMIT),
    FVM_CLIT(':'),
    FVM_OP(EMIT),
    FVM_OP(SPACE),
      FVM_OP(QDUP),
    FVM_OP(ZERO_BRANCH), 6,
      FVM_OP(DUP),
      FVM_OP(PICK),
      FVM_OP(DOT),
      FVM_OP(ONE_MINUS),
    FVM_OP(BRANCH), -9,
    FVM_OP(CR),
    FVM_OP(EXIT)
  };
  CALL(DOT_S_CODE);
#endif

  // .name ( x -- length )
  // Print operation/function name
  OP(DOT_NAME)
  {
    const __FlashStringHelper* s = NULL;
    if (tos < 128)
      s = (const __FlashStringHelper*) pgm_read_word(&opstr[tos]);
    else if (tos < 255)
      s = (const __FlashStringHelper*) pgm_read_word(&fnstr[tos-128]);
    tos = (s != NULL) ? ios.print(s) : 0;
  }
  NEXT();

  // ? ( addr -- ) @ . ;
  // Print value of variable
  OP(Q)
  static const code_t Q_CODE[] PROGMEM = {
    FVM_OP(FETCH),
    FVM_OP(DOT),
    FVM_OP(EXIT)
  };
  CALL(Q_CODE);

  // delay ( ms -- )
  // Yield while waiting given number of milli-seconds
  OP(DELAY)
  // : delay ( ms -- )
  //   millis >r
  //   begin
  //     millis r@ - over u<
  //   while
  //     yield
  //   repeat
  //   r> 2drop ;
  static const code_t DELAY_CODE[] PROGMEM = {
    FVM_OP(MILLIS),
    FVM_OP(TO_R),
      FVM_OP(MILLIS),
      FVM_OP(R_FETCH),
      FVM_OP(MINUS),
      FVM_OP(OVER),
      FVM_OP(U_LESS),
    FVM_OP(ZERO_BRANCH), 3,
      FVM_OP(YIELD),
    FVM_OP(BRANCH), -10,
    FVM_OP(R_FROM),
    FVM_OP(TWO_DROP),
    FVM_OP(EXIT)
  };
  CALL(DELAY_CODE);

  // micros ( -- us )
  // Micro-seconds
  OP(MICROS)
    *++sp = tos;
    tos = micros();
  NEXT();

  // millis ( -- ms )
  // Milli-seconds
  OP(MILLIS)
    *++sp = tos;
    tos = millis();
  NEXT();

  // pinmode ( mode pin -- )
  // Set digital pin mode
  OP(PINMODE)
    pinMode(tos, *sp--);
    tos = *sp--;
  NEXT();

  // digitalread ( pin -- state )
  // Read digital pin
  OP(DIGITALREAD)
    tos = digitalRead(tos);
  NEXT();

  // digitalwrite ( state pin -- )
  // Write digital pin
  OP(DIGITALWRITE)
    digitalWrite(tos, *sp--);
    tos = *sp--;
  NEXT();

  // digitaltoggle ( pin -- )
  // Toggle digital pin
  OP(DIGITALTOGGLE)
    digitalWrite(tos, !digitalRead(tos));
    tos = *sp--;
  NEXT();

  // analogread ( pin -- sample )
  // Read analog pin
  OP(ANALOGREAD)
    tos = analogRead(tos & 0xf);
  NEXT();

  // analogwrite ( n pin -- )
  // Write pwm pin
  OP(ANALOGWRITE)
    analogWrite(tos, *sp--);
    tos = *sp--;
  NEXT();

  // halt ( -- )
  // Halt virtual machine and save context
  OP(HALT)
    ip -= 1;

  // yield ( -- )
  // Yield virtual machine and save context
  OP(YIELD)
    *++sp = tos;
    task.m_sp = sp;
    task.m_ip = ip;
    task.m_rp = rp;
  return (ir == OP_YIELD);

  // fncall ( -- )
  // Call internal function (pointer in tp)
  FNCALL:
    *++rp = ip;
    ip = tp;

  // nop ( -- )
  // No operation
  OP(NOP)
  NEXT();

  default:
    ;
  }
  return (-1);
}


int FVM::execute(code_P fn, task_t& task)
{
  return (resume(task.call(fn)));
}

int FVM::execute(code_t op, task_t& task)
{
  static const code_t EXECUTE_CODE[] PROGMEM = {
    FVM_OP(EXECUTE),
    FVM_OP(HALT)
  };
  task.push((uint8_t) op);
  return (execute(EXECUTE_CODE, task));
}

int FVM::execute(const char* name, task_t& task)
{
  int op = lookup(name);
  if (op == -1) {
    task.push(atoi(name));
    op = OP_NOP;
  }
  return (execute(op, task));
}

#if defined(FVM_DICT)
static const char EXIT_PSTR[] PROGMEM = "exit";
static const char LITERAL_PSTR[] PROGMEM = "(literal)";
static const char C_LITERAL_PSTR[] PROGMEM = "(cliteral)";
static const char S_LITERAL_PSTR[] PROGMEM = "(sliteral)";
static const char VAR_PSTR[] PROGMEM = "(var)";
static const char CONST_PSTR[] PROGMEM = "(const)";
static const char DOES_PSTR[] PROGMEM = "(does)";
static const char PARAM_PSTR[] PROGMEM = "(param)";
static const char BRANCH_PSTR[] PROGMEM = "(branch)";
static const char ZERO_BRANCH_PSTR[] PROGMEM = "(0branch)";
static const char EXECUTE_PSTR[] PROGMEM = "execute";
static const char TRACE_PSTR[] PROGMEM = "trace";
static const char C_FETCH_PSTR[] PROGMEM = "c@";
static const char C_STORE_PSTR[] PROGMEM = "c!";
static const char FETCH_PSTR[] PROGMEM = "@";
static const char STORE_PSTR[] PROGMEM = "!";
static const char PLUS_STORE_PSTR[] PROGMEM = "+!";
static const char DP_PSTR[] PROGMEM = "dp";
static const char HERE_PSTR[] PROGMEM = "here";
static const char ALLOT_PSTR[] PROGMEM = "allot";
static const char COMMA_PSTR[] PROGMEM = ",";
static const char C_COMMA_PSTR[] PROGMEM = "c,";
static const char CELLS_PSTR[] PROGMEM = "cells";
static const char TO_R_PSTR[] PROGMEM = ">r";
static const char R_FROM_PSTR[] PROGMEM = "r>";
static const char R_FETCH_PSTR[] PROGMEM = "r@";
static const char QR_PSTR[] PROGMEM = "?r";
static const char SP_PSTR[] PROGMEM = "sp";
static const char DEPTH_PSTR[] PROGMEM = "depth";
static const char DROP_PSTR[] PROGMEM = "drop";
static const char NIP_PSTR[] PROGMEM = "nip";
static const char EMPTY_PSTR[] PROGMEM = "empty";
static const char DUP_PSTR[] PROGMEM = "dup";
static const char QDUP_PSTR[] PROGMEM = "?dup";
static const char OVER_PSTR[] PROGMEM = "over";
static const char TUCK_PSTR[] PROGMEM = "tuck";
static const char PICK_PSTR[] PROGMEM = "pick";
static const char SWAP_PSTR[] PROGMEM = "swap";
static const char ROT_PSTR[] PROGMEM = "rot";
static const char MINUS_ROT_PSTR[] PROGMEM = "-rot";
static const char ROLL_PSTR[] PROGMEM = "roll";
static const char TWO_SWAP_PSTR[] PROGMEM = "2swap";
static const char TWO_DUP_PSTR[] PROGMEM = "2dup";
static const char TWO_OVER_PSTR[] PROGMEM = "2over";
static const char TWO_DROP_PSTR[] PROGMEM = "2drop";
static const char CELL_PSTR[] PROGMEM = "cell";
static const char MINUS_TWO_PSTR[] PROGMEM = "-2";
static const char MINUS_ONE_PSTR[] PROGMEM = "-1";
static const char ZERO_PSTR[] PROGMEM = "0";
static const char ONE_PSTR[] PROGMEM = "1";
static const char TWO_PSTR[] PROGMEM = "2";
static const char BOOL_PSTR[] PROGMEM = "bool";
static const char NOT_PSTR[] PROGMEM = "not";
static const char TRUE_PSTR[] PROGMEM = "true";
static const char FALSE_PSTR[] PROGMEM = "false";
static const char INVERT_PSTR[] PROGMEM = "invert";
static const char AND_PSTR[] PROGMEM = "and";
static const char OR_PSTR[] PROGMEM = "or";
static const char XOR_PSTR[] PROGMEM = "xor";
static const char NEGATE_PSTR[] PROGMEM = "negate";
static const char ONE_PLUS_PSTR[] PROGMEM = "1+";
static const char ONE_MINUS_PSTR[] PROGMEM = "1-";
static const char TWO_PLUS_PSTR[] PROGMEM = "2+";
static const char TWO_MINUS_PSTR[] PROGMEM = "2-";
static const char TWO_STAR_PSTR[] PROGMEM = "2*";
static const char TWO_SLASH_PSTR[] PROGMEM = "2/";
static const char PLUS_PSTR[] PROGMEM = "+";
static const char MINUS_PSTR[] PROGMEM = "-";
static const char STAR_PSTR[] PROGMEM = "*";
static const char STAR_SLASH_PSTR[] PROGMEM = "*/";
static const char SLASH_PSTR[] PROGMEM = "/";
static const char MOD_PSTR[] PROGMEM = "mod";
static const char SLASH_MODE_PSTR[] PROGMEM = "/mod";
static const char LSHIFT_PSTR[] PROGMEM = "lshift";
static const char RSHIFT_PSTR[] PROGMEM = "rshift";
static const char ZERO_NOT_EQUALS_PSTR[] PROGMEM = "0<>";
static const char ZERO_LESS_PSTR[] PROGMEM = "0<";
static const char ZERO_EQUALS_PSTR[] PROGMEM = "0=";
static const char ZERO_GREATER_PSTR[] PROGMEM = "0>";
static const char NOT_EQUALS_PSTR[] PROGMEM = "<>";
static const char LESS_PSTR[] PROGMEM = "<";
static const char EQUALS_PSTR[] PROGMEM = "=";
static const char GREATER_PSTR[] PROGMEM = ">";
static const char U_LESS_PSTR[] PROGMEM = "u<";
static const char WITHIN_PSTR[] PROGMEM = "within";
static const char ABS_PSTR[] PROGMEM = "abs";
static const char MIN_PSTR[] PROGMEM = "min";
static const char MAX_PSTR[] PROGMEM = "max";
static const char LOOKUP_PSTR[] PROGMEM = "lookup";
static const char TO_BODY_PSTR[] PROGMEM = ">body";
static const char WORDS_PSTR[] PROGMEM = "words";
static const char BASE_PSTR[] PROGMEM = "base";
static const char HEX_PSTR[] PROGMEM = "hex";
static const char DECIMAL_PSTR[] PROGMEM = "decimal";
static const char QKEY_PSTR[] PROGMEM = "?key";
static const char KEY_PSTR[] PROGMEM = "key";
static const char EMIT_PSTR[] PROGMEM = "emit";
static const char CR_PSTR[] PROGMEM = "cr";
static const char SPACE_PSTR[] PROGMEM = "space";
static const char SPACES_PSTR[] PROGMEM = "spaces";
static const char U_DOT_PSTR[] PROGMEM = "u.";
static const char DOT_PSTR[] PROGMEM = ".";
static const char DOT_S_PSTR[] PROGMEM = ".s";
static const char DOT_NAME_PSTR[] PROGMEM = ".name";
static const char Q_PSTR[] PROGMEM = "?";
static const char MICROS_PSTR[] PROGMEM = "micros";
static const char MILLIS_PSTR[] PROGMEM = "millis";
static const char DELAY_PSTR[] PROGMEM = "delay";
static const char PINMODE_PSTR[] PROGMEM = "pinmode";
static const char DIGITALREAD_PSTR[] PROGMEM = "digitalread";
static const char DIGITALWRITE_PSTR[] PROGMEM = "digitalwrite";
static const char DIGITALTOGGLE_PSTR[] PROGMEM = "digitaltoggle";
static const char ANALOGREAD_PSTR[] PROGMEM = "analogread";
static const char ANALOGWRITE_PSTR[] PROGMEM = "analogwrite";
static const char HALT_PSTR[] PROGMEM = "halt";
static const char YIELD_PSTR[] PROGMEM = "yield";
static const char NOP_PSTR[] PROGMEM = "nop";
#endif

const str_P FVM::opstr[] PROGMEM = {
#if defined(FVM_DICT)
  (str_P) EXIT_PSTR,
  (str_P) LITERAL_PSTR,
  (str_P) C_LITERAL_PSTR,
  (str_P) S_LITERAL_PSTR,
  (str_P) VAR_PSTR,
  (str_P) CONST_PSTR,
  (str_P) DOES_PSTR,
  (str_P) PARAM_PSTR,
  (str_P) BRANCH_PSTR,
  (str_P) ZERO_BRANCH_PSTR,
  (str_P) EXECUTE_PSTR,
  (str_P) TRACE_PSTR,
  (str_P) C_FETCH_PSTR,
  (str_P) C_STORE_PSTR,
  (str_P) FETCH_PSTR,
  (str_P) STORE_PSTR,
  (str_P) PLUS_STORE_PSTR,
  (str_P) DP_PSTR,
  (str_P) HERE_PSTR,
  (str_P) ALLOT_PSTR,
  (str_P) COMMA_PSTR,
  (str_P) C_COMMA_PSTR,
  (str_P) CELLS_PSTR,
  (str_P) TO_R_PSTR,
  (str_P) R_FROM_PSTR,
  (str_P) R_FETCH_PSTR,
  (str_P) QR_PSTR,
  (str_P) SP_PSTR,
  (str_P) DEPTH_PSTR,
  (str_P) DROP_PSTR,
  (str_P) NIP_PSTR,
  (str_P) EMPTY_PSTR,
  (str_P) DUP_PSTR,
  (str_P) QDUP_PSTR,
  (str_P) OVER_PSTR,
  (str_P) TUCK_PSTR,
  (str_P) PICK_PSTR,
  (str_P) SWAP_PSTR,
  (str_P) ROT_PSTR,
  (str_P) MINUS_ROT_PSTR,
  (str_P) ROLL_PSTR,
  (str_P) TWO_SWAP_PSTR,
  (str_P) TWO_DUP_PSTR,
  (str_P) TWO_OVER_PSTR,
  (str_P) TWO_DROP_PSTR,
  (str_P) CELL_PSTR,
  (str_P) MINUS_TWO_PSTR,
  (str_P) MINUS_ONE_PSTR,
  (str_P) ZERO_PSTR,
  (str_P) ONE_PSTR,
  (str_P) TWO_PSTR,
  (str_P) BOOL_PSTR,
  (str_P) NOT_PSTR,
  (str_P) TRUE_PSTR,
  (str_P) FALSE_PSTR,
  (str_P) INVERT_PSTR,
  (str_P) AND_PSTR,
  (str_P) OR_PSTR,
  (str_P) XOR_PSTR,
  (str_P) NEGATE_PSTR,
  (str_P) ONE_PLUS_PSTR,
  (str_P) ONE_MINUS_PSTR,
  (str_P) TWO_PLUS_PSTR,
  (str_P) TWO_MINUS_PSTR,
  (str_P) TWO_STAR_PSTR,
  (str_P) TWO_SLASH_PSTR,
  (str_P) PLUS_PSTR,
  (str_P) MINUS_PSTR,
  (str_P) STAR_PSTR,
  (str_P) STAR_SLASH_PSTR,
  (str_P) SLASH_PSTR,
  (str_P) MOD_PSTR,
  (str_P) SLASH_MODE_PSTR,
  (str_P) LSHIFT_PSTR,
  (str_P) RSHIFT_PSTR,
  (str_P) WITHIN_PSTR,
  (str_P) ABS_PSTR,
  (str_P) MIN_PSTR,
  (str_P) MAX_PSTR,
  (str_P) ZERO_NOT_EQUALS_PSTR,
  (str_P) ZERO_LESS_PSTR,
  (str_P) ZERO_EQUALS_PSTR,
  (str_P) ZERO_GREATER_PSTR,
  (str_P) NOT_EQUALS_PSTR,
  (str_P) LESS_PSTR,
  (str_P) EQUALS_PSTR,
  (str_P) GREATER_PSTR,
  (str_P) U_LESS_PSTR,
  (str_P) LOOKUP_PSTR,
  (str_P) TO_BODY_PSTR,
  (str_P) WORDS_PSTR,
  (str_P) BASE_PSTR,
  (str_P) HEX_PSTR,
  (str_P) DECIMAL_PSTR,
  (str_P) QKEY_PSTR,
  (str_P) KEY_PSTR,
  (str_P) EMIT_PSTR,
  (str_P) CR_PSTR,
  (str_P) SPACE_PSTR,
  (str_P) SPACES_PSTR,
  (str_P) U_DOT_PSTR,
  (str_P) DOT_PSTR,
  (str_P) DOT_S_PSTR,
  (str_P) DOT_NAME_PSTR,
  (str_P) Q_PSTR,
  (str_P) MICROS_PSTR,
  (str_P) MILLIS_PSTR,
  (str_P) DELAY_PSTR,
  (str_P) PINMODE_PSTR,
  (str_P) DIGITALREAD_PSTR,
  (str_P) DIGITALWRITE_PSTR,
  (str_P) DIGITALTOGGLE_PSTR,
  (str_P) ANALOGREAD_PSTR,
  (str_P) ANALOGWRITE_PSTR,
  (str_P) HALT_PSTR,
  (str_P) YIELD_PSTR,
  (str_P) NOP_PSTR,
#endif
  0
};
