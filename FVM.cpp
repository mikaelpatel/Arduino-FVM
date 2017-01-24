/**
 * @file FVM.cpp
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
 */

#include "FVM.h"

/**
 * Enable threaded code in data memory.
 * 0: Program memory only.
 * 1: Data and program memory.
 */
#define FVM_THREADING 1

/**
 * Enable symbolic trace of virtual machine instruction cycle.
 * 0: No trace.
 * 1: Print indented operation code and stack contents.
 * 2: Print execute time, instruction pointer, return stack depth,
 * operation code and stack contents.
 */
#define FVM_TRACE 1

/**
 * Enable kernel dictionary. Remove to reduce foot-print for
 * non-interactive application.
 * 0: No kernel dictionary.
 * 1: Add kernel dictionary
 */
#define FVM_KERNEL_DICT 1

/**
 * Enable kernel optimization.
 * 0: No kernel optimization.
 * 1: Tail call optimization.
 */
#define FVM_KERNEL_OPT 1

// Forth Virtual Machine support macros
#define OP(n) case OP_ ## n:
#define NEXT() goto INNER
#define FALLTHROUGH()
#define CALL(fn) tp = fn; goto FNCALL
#define FNTAB(ir) (code_P) pgm_read_word(fntab-ir-1)
#define FNSTR(ir) (const __FlashStringHelper*) pgm_read_word(fnstr-ir-1)
#define OPSTR(ir) (const __FlashStringHelper*) pgm_read_word(opstr+ir)

// Configurate for threading program memory only or also data memory
#if (FVM_THREADING == 0)

#define fetch_byte(ip) (int8_t) pgm_read_byte(ip)
#define fetch_word(ip) (cell_t) pgm_read_word(ip)

#else

int8_t fetch_byte(FVM::code_P ip)
{
  if (ip < (FVM::code_P) FVM::CODE_P_MAX)
    return ((int8_t) pgm_read_byte(ip));
  return (*(((int8_t*) ip) - FVM::CODE_P_MAX));
}

FVM::cell_t fetch_word(FVM::code_P ip)
{
  if (ip < (FVM::code_P) FVM::CODE_P_MAX)
    return ((FVM::cell_t) pgm_read_word(ip));
  return (*(FVM::cell_t*) (((int8_t*) ip) - FVM::CODE_P_MAX));
}

#endif

int FVM::lookup(const char* name)
{
  const char* s;

  // Search dynamic sketch dictionary, return index
  for (int i = 0; i < m_next; i++)
    if (!strcmp(name, m_name[i])) return (i + FVM::APPLICATION_MAX);

  // Search static sketch dictionary, return index
  for (int i = 0; (s = (const char*) pgm_read_word(&fnstr[i])) != 0; i++)
    if (!strcmp_P(name, s)) return (i + FVM::KERNEL_MAX);

  // Search static kernel dictionary, return index
  for (int i = 0; (s = (const char*) pgm_read_word(&opstr[i])) != 0; i++)
    if (!strcmp_P(name, s)) return (i);

  // Return error code
  return (-1);
}

int FVM::scan(char* bp, task_t& task)
{
  Stream& ios = task.m_ios;
  char c;

  // Skip white space (blocking)
  do {
    while (!ios.available());
    c = ios.read();
  } while (c <= ' ');

  // Scan until white space (blocking)
  do {
    *bp++ = c;
    while (!ios.available());
    c = ios.read();
   } while (c > ' ');
  *bp = 0;

  return (c);
}

int FVM::resume(task_t& task)
{
  // Restore virtual machine state
  Stream& ios = task.m_ios;
  const code_t** rp = task.m_rp;
  const code_t* ip = *rp--;
  cell_t* sp = task.m_sp;
  cell_t tos = *sp--;
  const code_t* tp;
  cell_t tmp;
  int8_t ir;

  // Benchmark support in trace mode; measure micro-seconds per operation
#if (FVM_TRACE == 2)
  uint32_t start = micros();
#endif

 INNER:
  // Positive opcode (0..127) are direct operation codes. Negative
  // opcodes (-1..-128) are negative index (plus one) in threaded code
  // table. Direct operation codes may be implemented as a primitive
  // or as an internal threaded code call.
  //
  // The virtual machine allows 512 tokens. These are used as follows:
  // Kernel tokens 0..255: 0..127 direct, 128..255 OP_KERNEL prefix.
  // Application tokens 256..511: 256..383 direct, -1..-128, indexing
  // threaded code table in program memory 0..127, 384..511, indexing
  // threaded code table in data memory 0..127 OP_CALL prefix.
  //
  // Kernel inner may be configured for tail call optimization.
  //
  // Kernel operations are documented according to ANSI X3.215-1994,
  // American National Standard for Information Systems, Programming
  // Languages - Forth, March 24, 1994.
#if (FVM_TRACE == 0)

  while ((ir = fetch_byte(ip++)) < 0) {
#if (FVM_KERNEL_OPT == 1)
    if (fetch_byte(ip)) *++rp = ip;
#else
    *++rp = ip;
#endif
    ip = FNTAB(ir);
  }

#else
  // Trace execution; micro-seconds, instruction pointer,
  // return stack depth, token, and stack contents
  do {
    if (task.m_trace) {
#if (FVM_TRACE == 2)
      uint32_t stop = micros();
#endif
      ios.print(F("task@"));
      ios.print((uint16_t) &task);
      ios.print(':');
#if (FVM_TRACE == 2)
      // Print measurement of latest operation; micro-seconds
      ios.print(stop - start);
      ios.print(':');
      // Print current instruction pointer
      ios.print((uint16_t) ip);
      ios.print(':');
      // Print current return stack depth
      ios.print((uint16_t) (rp - task.m_rp0));
      ios.print(':');
#else
      uint16_t depth = (uint16_t) (rp - task.m_rp0);
      while (depth--) ios.print(' ');
#endif
    }
    // Fetch next instruction and check for threaded call or primitive
    // Print name or token
    ir = fetch_byte(ip++);
    if (ir < 0 ) {
#if (FVM_KERNEL_OPT == 1)
      if (fetch_byte(ip)) *++rp = ip;
#else
      *++rp = ip;
#endif
      ip = FNTAB(ir);
      if (task.m_trace) {
#if (FVM_KERNEL_DICT == 0)
	ios.print(KERNEL_MAX-ir-1);
#else
	ios.print(FNSTR(ir));
#endif
      }
    }
    else if (task.m_trace) {
#if (FVM_KERNEL_DICT == 0)
      ios.print(ir);
#else
#if (FVM_THREADING == 1)
      if (ir == OP_CALL)
	ios.print(m_name[(uint8_t) fetch_byte(ip)]);
      else if (ir == OP_KERNEL)
	ios.print(OPSTR((uint8_t) fetch_byte(ip)));
      else
#endif
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
	cell_t* tp = task.m_sp0 + 1;
	while (--tmp) {
	  ios.print(*++tp);
	  ios.print(' ');
	}
	ios.print(tos);
      }
      ios.println();
    }
    // Flush output and start measurement
    ios.flush();
#if (FVM_TRACE == 2)
    if (task.m_trace) start = micros();
#endif
  } while (ir < 0);
#endif

  // Dispatch instruction; primitive or internal threaded code call
DISPATCH:
  switch ((uint8_t) ir) {

  // ?exit ( flag -- )
  // Exit from call if zero/false.
  OP(ZERO_EXIT)
    tmp = tos;
    tos = *sp--;
    if (tmp != 0) NEXT();
  FALLTHROUGH();

  // exit ( -- ) ( R: nest-sys -- )
  // Return control to the calling definition specified by nest-sys.
  OP(EXIT)
    ip = *rp--;
  NEXT();

  // (lit) ( -- x )
  // Push literal data (little-endian).
  OP(LIT)
    *++sp = tos;
    tos = (uint8_t) fetch_byte(ip++);
    tos |= (fetch_byte(ip++) << 8);
  NEXT();

  // (clit) ( -- x )
  // Push literal data (signed byte).
  OP(CLIT)
    *++sp = tos;
    tos = fetch_byte(ip++);
  NEXT();

  // (var) ( -- addr )
  // Push address of variable (pointer to cell).
  OP(VAR)
    *++sp = tos;
    tos = (cell_t) (ip - CODE_P_MAX);
    ip = *rp--;
  NEXT();

  // (const) ( -- value )
  // Push value of contant.
  OP(CONST)
    *++sp = tos;
    tos = fetch_word(ip);
    ip = *rp--;
  NEXT();

  // (func) ( xn..x0 -- ym..y0 )
  // Call extension function wrapper.
  OP(FUNC)
  {
    void* env = (void*) fetch_word(ip + sizeof(fn_t));
    fn_t fn = (fn_t) fetch_word(ip);
    *++sp = tos;
    task.m_sp = sp;
    task.m_rp = rp;
    fn(task, env);
    rp = task.m_rp;
    sp = task.m_sp;
    tos = *sp--;
    ip = *rp--;
  }
  NEXT();

  // (does) ( -- addr )
  // Push object pointer accessed by return address.
  OP(DOES)
    *++sp = tos;
    tp = *rp--;
    tos = fetch_word(tp + 1);
  NEXT();

  // (param) ( xn..x0 -- xn..x0 xi )
  // Duplicate inline index stack element to top of stack.
  OP(PARAM)
    *++sp = tos;
    ir = fetch_byte(ip++);
    tos = *(sp - ir);
  NEXT();

  // (slit) ( -- addr )
  // Push pointer to literal and branch.
  OP(SLIT)
    *++sp = tos;
    tos = (cell_t) ip + 1;

  // (branch) ( -- )
  // Branch always (8-bit offset, -128..127).
  OP(BRANCH)
    ir = fetch_byte(ip);
    ip += ir;
  NEXT();

  // (0branch) ( flag -- )
  // Branch zero equal/false (8-bit offset, -128..127).
  OP(ZERO_BRANCH)
    ir = fetch_byte(ip);
    ip += (tos == 0) ? ir : 1;
    tos = *sp--;
  NEXT();

  // (do) ( n1|u1 n2|u2 -- ) ( R: -- loop-sys )
  // Set up loop control parameters with index n2|u2 and limit
  // n1|u1. An ambiguous condition exists if n1|u1 and n2|u2 are not
  // both the same type. Anything already on the return stack becomes
  // unavailable until the loop-control parameters are discarded.
  OP(DO)
    tmp = *sp--;
    if (tos != tmp) {
      *++rp = (code_t*) tmp;
      *++rp = (code_t*) tos;
      ip += 1;
    }
    else {
      ir = fetch_byte(ip);
      ip += ir;
    }
    tos = *sp--;
  NEXT();

  // j ( -- n|u ) ( R: loop-sys1 loop-sys2 -- loop-sys1 loop-sys2 )
  // n|u is a copy of the next-outer loop index. An ambiguous
  // condition exists if the loop control parameters of the next-outer
  // loop, loop-sys1, are unavailable.
  OP(J)
    *++sp = tos;
    tos = (cell_t) *(rp - 2);
  NEXT();

  // leave ( -- rp: high high )
  // Mark loop block as completed.
  OP(LEAVE)
    *rp = *(rp - 1);
  NEXT();

  // (loop) ( -- ) ( R: loop-sys1 -- | loop-sys2 )
  // An ambiguous condition exists if the loop control parameters are
  // unavailable. Add one to the loop index. If the loop index is then
  // equal to the loop limit, discard the loop parameters and continue
  // execution immediately following the loop. Otherwise continue
  // execution at the beginning of the loop.
  OP(LOOP)
    *rp += 1;
    if (*rp < *(rp - 1)) {
      ir = fetch_byte(ip);
      ip += ir;
    }
    else {
      rp -= 2;
      ip += 1;
    }
  NEXT();

  // (+loop) ( n -- )
  // Add n to the loop index. If the loop index did not cross the
  // boundary between the loop limit minus one and the loop limit,
  // continue execution at the beginning of the loop. Otherwise,
  // discard the current loop control parameters and continue
  // execution immediately following the loop.
  OP(PLUS_LOOP)
    *rp += tos;
    if (*rp < *(rp - 1)) {
      ir = fetch_byte(ip);
      ip += ir;
    }
    else {
      rp -= 2;
      ip += 1;
    }
    tos = *sp--;
  FALLTHROUGH();

  // noop ( -- )
  // No operation.
  OP(NOOP)
  NEXT();

  // execute ( i*x xt -- j*x )
  // Remove xt from the stack and perform the semantics identified by
  // it. Other stack effects are due to the semantics of the token.
  OP(EXECUTE)
    if (tos < KERNEL_MAX) {
      ir = tos;
      tos = *sp--;
      goto DISPATCH;
    }
    else if (tos < APPLICATION_MAX) {
      *++rp = ip;
      ip = (code_P) pgm_read_word(fntab+(tos-KERNEL_MAX));
      tos = *sp--;
    }
    else {
      *++rp = ip;
      ip = (code_P) m_body[tos - APPLICATION_MAX];
      tos = *sp--;
    }
  NEXT();

  // halt ( -- )
  // Halt virtual machine. Do not proceed on resume.
  OP(HALT)
    rp = task.m_rp0;
    ip -= 1;
  FALLTHROUGH();

  // yield ( -- )
  // Yield virtual machine.
  OP(YIELD)
    *++sp = tos;
    *++rp = ip;
    task.m_sp = sp;
    task.m_rp = rp;
  return (ir == OP_YIELD);

  // (kernel) ( -- )
  // Call inline kernel token (0..255); compiled code.
  OP(KERNEL)
    ir = fetch_byte(ip++);
  goto DISPATCH;

  // (call) ( -- )
  // Call application token in dynamic dictionary; 384..255
  // are mapped to 0..127.
  OP(CALL)
    tmp = (uint8_t) fetch_byte(ip++);
#if (FVM_KERNEL_OPT == 1)
      if (fetch_byte(ip)) *++rp = ip;
#else
      *++rp = ip;
#endif
    ip = (code_P) m_body[tmp];
  NEXT();

  // trace ( flag -- )
  // Set trace mode.
  OP(TRACE)
    task.m_trace = tos;
    tos = *sp--;
  NEXT();

  // room ( -- n bytes )
  // Number of free dictionary entries and bytes.
  OP(ROOM)
    *++sp = tos;
    *++sp = WORD_MAX - m_next;
    tos = DICT_MAX - (m_dp - (uint8_t*) m_body);
  NEXT();

  // c@ ( c-addr -- char )
  // Fetch the character stored at c-addr. When the cell size is
  // greater than character size, the unused high-order bits are all
  // zeroes.
  OP(C_FETCH)
    tos = *((uint8_t*) tos);
  NEXT();

  // c! ( char c-addr -- )
  // Store char at c-addr. When character size is smaller than cell
  // size, only the number of low-order bits corresponding to
  // character size are transferred.
  OP(C_STORE)
    *((uint8_t*) tos) = *sp--;
    tos = *sp--;
  NEXT();

  // @ ( a-addr -- x )
  // x is the value stored at a-addr.
  OP(FETCH)
    tos = *((cell_t*) tos);
  NEXT();

  // ! ( x a-addr -- )
  // Store x at a-addr.
  OP(STORE)
    *((cell_t*) tos) = *sp--;
    tos = *sp--;
  NEXT();

  // +! ( n|u a-addr -- )
  // Add n|u to the single-cell number at a-addr.
  OP(PLUS_STORE)
#if 0
    *((cell_t*) tos) += *sp--;
    tos = *sp--;
  NEXT();
#else
  // : +! ( n|u a-addr -- ) dup >r @ + r> ! ;
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

  // dp ( -- a-addr )
  // Push address to data-space pointer.
  OP(DP)
    *++sp = tos;
    tos = (cell_t) &m_dp;
  NEXT();

  // here ( -- a-addr )
  // a-addr is the data-space pointer.
  OP(HERE)
#if 0
    *++sp = tos;
    tos = (cell_t) m_dp;
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
  // If n is greater than zero, reserve n address units of data
  // space. If n is less than zero, release |n| address units of data
  // space. If n is zero, leave the data-space pointer unchanged.
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
  // Reserve one cell of data space and store x in the cell.
  OP(COMMA)
#if 0
    *((cell_t*) m_dp) = tos;
    m_dp += sizeof(cell_t);
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

  // c, ( char -- )
  // Reserve space for one character in the data space and store char
  // in the space.
  OP(C_COMMA)
#if 0
    *m_dp++ = tos;
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

  // (compile) ( -- )
  // Add inline token (0..255) to compile stream.
  OP(COMPILE)
    *m_dp++ = fetch_byte(ip++);
  NEXT();

  // >r ( x -- ) ( R: -- x )
  // Move x to the return stack.
  OP(TO_R)
    *++rp = (code_t*) tos;
    tos = *sp--;
  NEXT();

  // r> ( -- x ) ( R: x -- )
  // Move x from the return stack to the data stack.
  OP(R_FROM)
    *++sp = tos;
    tos = (cell_t) *rp--;
  NEXT();

  // i ( -- n|u ) ( R: loop-sys -- loop-sys )
  // n|u is a copy of the current (innermost) loop index. An ambiguous
  // condition exists if the loop control parameters are unavailable.
  OP(I)
  FALLTHROUGH();

  // r@ ( -- x ) ( R: x -- x )
  // Copy x from the return stack to the data stack.
  OP(R_FETCH)
    *++sp = tos;
    tos = (cell_t) *rp;
  NEXT();

  // sp ( -- addr )
  // Push stack pointer.
  OP(SP)
    *++sp = tos;
    tos = (cell_t) sp;
  NEXT();

  // depth ( -- +n )
  // +n is the number of single-cell values contained in the data
  // stack before +n was placed on the stack.
  OP(DEPTH)
    tmp = (sp - task.m_sp0);
    *++sp = tos;
    tos = tmp;
  NEXT();

  // drop ( x -- )
  // Remove x from the stack.
  OP(DROP)
    tos = *sp--;
  NEXT();

  // nip ( x1 x2 -- x2 )
  // Drop the first item below the top of stack.
  OP(NIP)
#if 1
    sp -= 1;
  NEXT();
#else
  // : nip ( x1 x2 -- x2 ) swap drop ;
  static const code_t NIP_CODE[] PROGMEM = {
    FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(NIP_CODE);
#endif

  // empty ( xn...x0 -- )
  // Empty data stack.
  OP(EMPTY)
    sp = task.m_sp0;
  NEXT();

  // dup ( x -- x x )
  // Duplicate x.
  OP(DUP)
#if 1
    *++sp = tos;
  NEXT();
#else
  // : dup ( x -- x x ) param: 0  ;
  static const code_t DUP_CODE[] PROGMEM = {
    FVM_OP(PARAM), 0,
    FVM_OP(EXIT)
  };
  CALL(DUP_CODE);
#endif

  // ?dup ( x -- 0 | x x )
  // Duplicate x if it is non-zero.
  OP(QUESTION_DUP)
#if 1
    if (tos != 0) *++sp = tos;
  NEXT();
#else
  // : ?dup ( x -- 0 | x x ) dup ?exit dup ;
  static const code_t QUESTION_DUP_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(ZERO_EXIT),
    FVM_OP(DUP),
    FVM_OP(EXIT)
  };
  CALL(QUESTION_DUP_CODE);
#endif

  // over ( x1 x2 -- x1 x2 x1 )
  // Place a copy of x 1 on top of the stack.
  OP(OVER)
#if 1
    tmp = *sp;
    *++sp = tos;
    tos = tmp;
  NEXT();
#else
  // : over ( x1 x2 -- x1 x2 x1 ) param: 1 ;
  static const code_t OVER_CODE[] PROGMEM = {
    FVM_OP(PARAM), 1,
    FVM_OP(EXIT)
  };
  CALL(OVER_CODE);
#endif

  // tuck ( x1 x2 -- x2 x1 x2 )
  // Copy the first (top) stack item below the second stack item.
  OP(TUCK)
#if 0
    tmp = *sp;
    *sp = tos;
    *++sp = tmp;
  NEXT();
#else
  // : tuck ( x1 x2 -- x2 x1 x2 ) swap over ;
  static const code_t TUCK_CODE[] PROGMEM = {
    FVM_OP(SWAP),
    FVM_OP(OVER),
    FVM_OP(EXIT)
  };
  CALL(TUCK_CODE);
#endif

  // pick ( xn..x0 i -- xn..x0 xi )
  // Duplicate index stack element to top of stack.
  OP(PICK)
    tos = *(sp - tos);
  NEXT();

  // swap ( x1 x2 -- x2 x1 )
  // Exchange the top two stack items.
  OP(SWAP)
#if 1
    tmp = tos;
    tos = *sp;
    *sp = tmp;
  NEXT();
#else
  // : swap ( x1 x2 -- x2 x1 ) 1 roll ;
  static const code_t SWAP_CODE[] PROGMEM = {
    FVM_OP(ONE),
    FVM_OP(ROLL),
    FVM_OP(EXIT)
  };
  CALL(SWAP_CODE);
#endif

  // rot ( x1 x2 x3 -- x2 x3 x1 )
  // Rotate the top three stack entries.
  OP(ROT)
#if 1
    tmp = tos;
    tos = *(sp - 1);
    *(sp - 1) = *sp;
    *sp = tmp;
  NEXT();
#else
  // : rot ( x1 x2 x3 -- x2 x3 x1 ) 2 roll ;
  static const code_t ROT_CODE[] PROGMEM = {
    FVM_OP(TWO),
    FVM_OP(ROLL),
    FVM_OP(EXIT)
  };
  CALL(ROT_CODE);
#endif

  // -rot ( x1 x2 x3 -- x3 x1 x2 )
  // Rotate down top three stack elements.
  OP(MINUS_ROT)
#if 0
    tmp = tos;
    tos = *sp;
    *sp = *(sp - 1);
    *(sp - 1) = tmp;
  NEXT();
#else
  // : -rot ( x1 x2 x3 -- x3 x1 x2 ) rot rot ;
  static const code_t MINUS_ROT_CODE[] PROGMEM = {
    FVM_OP(ROT),
    FVM_OP(ROT),
    FVM_OP(EXIT)
  };
  CALL(MINUS_ROT_CODE);
#endif

  // roll ( xn..x0 n -- xn-1..x0 xn )
  // Rotate up n+1 stack elements.
  OP(ROLL)
    tmp = tos;
    tos = sp[-tmp];
    for (; tmp > 0; tmp--)
      sp[-tmp] = sp[-tmp + 1];
    sp -= 1;
  NEXT();

  // : 2swap ( x1 x2 x3 x4 -- x3 x4 x1 x2 ) rot >r rot r> ;
  // Exchange the top two cell pairs.
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
  // Duplicate cell pair x1 x2.
  OP(TWO_DUP)
  static const code_t TWO_DUP_CODE[] PROGMEM = {
    FVM_OP(OVER),
    FVM_OP(OVER),
    FVM_OP(EXIT)
  };
  CALL(TWO_DUP_CODE);

  // : 2over ( x1 x2 y1 y2 -- x1 y1 y1 y2 x1 x2 ) param: 3 param: 3 ;
  // Copy cell pair x1 x2 to the top of the stack.
  OP(TWO_OVER)
  static const code_t TWO_OVER_CODE[] PROGMEM = {
    FVM_OP(PARAM), 3,
    FVM_OP(PARAM), 3,
    FVM_OP(EXIT)
  };
  CALL(TWO_OVER_CODE);

  // : 2drop ( x1 x2 -- ) drop drop ;
  // Drop cell pair x1 x2 from the stack.
  OP(TWO_DROP)
  static const code_t TWO_DROP_CODE[] PROGMEM = {
    FVM_OP(DROP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(TWO_DROP_CODE);

  // -2 ( -- -2 )
  // Constant -2.
  OP(MINUS_TWO)
    *++sp = tos;
    tos = -2;
  NEXT();

  // -1 ( -- -1 )
  // Constant -1.
  OP(MINUS_ONE)
  FALLTHROUGH();

  // TRUE ( -- -1 )
  // Constant true (alias -1).
  OP(TRUE)
    *++sp = tos;
    tos = -1;
  NEXT();

  // 0 ( -- 0 )
  // Constant 0.
  OP(ZERO)
  FALLTHROUGH();

  // FALSE ( -- 0 )
  // Constant false.
  OP(FALSE)
    *++sp = tos;
    tos = 0;
  NEXT();

  // 1 ( -- 1 )
  // Constant 1.
  OP(ONE)
    *++sp = tos;
    tos = 1;
  NEXT();

  // 2 ( -- 2 )
  // Constant 2.
  OP(TWO)
    *++sp = tos;
    tos = 2;
  NEXT();

  // cell ( -- n )
  // Size of data element in bytes.
  OP(CELL)
    *++sp = tos;
    tos = sizeof(cell_t);
  NEXT();

  // cells ( x -- y )
  // Convert cells to bytes for allot.
  OP(CELLS)
#if 1
    tos *= sizeof(cell_t);
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

  // invert ( x1 -- x2 )
  // Invert all bits of x1, giving its logical inverse x2.
  OP(INVERT)
    tos = ~tos;
  NEXT();

  // and ( x1 x2 -- x3 )
  // x3 is the bit-by-bit logical “and” of x1 with x2.
  OP(AND)
    tos = *sp-- & tos;
  NEXT();

  // or ( x1 x2 -- x3 )
  // x3 is the bit-by-bit inclusive-or of x1 with x2.
  OP(OR)
    tos = *sp-- | tos;
  NEXT();

  // xor ( x1 x2 -- x3 )
  // x3 is the bit-by-bit exclusive-or of x1 with x2.
  OP(XOR)
    tos = *sp-- ^ tos;
  NEXT();

  // negate ( n1 -- n2 )
  // Negate n1, giving its arithmetic inverse n2.
  OP(NEGATE)
#if 1
    tos = -tos;
  NEXT();
#else
  // : negate ( n1 -- n2 ) invert 1+ ;
  static const code_t NEGATE_CODE[] PROGMEM = {
    FVM_OP(INVERT),
    FVM_OP(ONE_PLUS),
    FVM_OP(EXIT)
  };
  CALL(NEGATE_CODE);
#endif

  // 1+ ( n1|u1 -- n2|u2 )
  // Add one (1) to n1|u1 giving the sum n2|u2.
  OP(ONE_PLUS)
    tos += 1;
  NEXT();

  // 1- ( n1|u1 -- n2|u2 )
  // Subtract one (1) from n1|u1 giving the difference n2|u2.
  OP(ONE_MINUS)
    tos -= 1;
  NEXT();

  // 2+ ( n1|u1 -- n2|u2 )
  // Add two (2) to n1|u1 giving the sum n2|u2.
  OP(TWO_PLUS)
    tos += 2;
  NEXT();

  // 2- ( n1|u1 -- n2|u2 )
  // Subtract two (2) from n1|u1 giving the difference n2|u2.
  OP(TWO_MINUS)
    tos -= 2;
  NEXT();

  // 2* ( x1 -- x2 )
  // x2 is the result of shifting x1 one bit toward the most-
  // significant bit, filling the vacated least-significant bit
  // with zero.
  OP(TWO_STAR)
    tos <<= 1;
  NEXT();

  // 2/ ( x1 -- x2 )
  // x2 is the result of shifting x1 one bit toward the
  // least-significant bit, leaving the most-significant bit
  // unchanged.
  OP(TWO_SLASH)
    tos >>= 1;
  NEXT();

  // + ( n1|u1 n2|u2 -- n3|u3 )
  // Add n2|u2 to n1|u1, giving the sum n3|u3.
  OP(PLUS)
    tos = *sp-- + tos;
  NEXT();

  // - ( n1|u1 n2|u2 -- n3|u3 )
  // Subtract n2|u2 from n1|u1, giving the difference n3|u3.
  OP(MINUS)
    tos = *sp-- - tos;
  NEXT();

  // * ( n1|u1 n2|u2 -- n3|u3 )
  // Multiply n1|u1 by n2|u2 giving the product n3|u3.
  OP(STAR)
    tos = *sp-- * tos;
  NEXT();

  // */ ( n1 n2 n3 -- n4 )
  // Multiply n1 by n2 producing the intermediate double-cell result
  // d. Divide d by n3 giving the single-cell quotient n4. An
  // ambiguous condition exists if n3 is zero or if the quotient n4
  // lies outside the range of a signed number.
  OP(STAR_SLASH)
    tmp = *sp--;
    tos = (((cell2_t) tmp) * (*sp--)) / tos;
  NEXT();

  // / ( n1 n2 -- n3 )
  // Divide n1 by n2, giving the single-cell quotient n3. An
  // ambiguous condition exists if n2 is zero.
  OP(SLASH)
    tos = *sp-- / tos;
  NEXT();

  // mod ( n1 n2 -- n3 )
  // Divide n1 by n2, giving the single-cell remainder n3. An
  // ambiguous condition exists if n2 is zero.
  OP(MOD)
    tos = *sp-- % tos;
  NEXT();

  // /mod ( n1 n2 -- n3 n4 )
  // Divide n1 by n2, giving the single-cell remainder n3 and the
  // single-cell quotient n4. An ambiguous condition exists if n2 is
  // zero.
  OP(SLASH_MOD)
    tmp = *sp / tos;
    tos = *sp % tos;
    *sp = tmp;
  NEXT();

  // lshift ( x1 u -- x2 )
  // Perform a logical left shift of u bit-places on x1, giving
  // x2. Put zeroes into the least significant bits vacated by the
  // shift. An ambiguous condition exists if u is greater than or
  // equal to the number of bits in a cell.
  OP(LSHIFT)
    tos = *sp-- << tos;
  NEXT();

  // rshift ( x1 u -- x2 )
  // Perform a logical right shift of u bit-places on x1, giving x2.
  // Put zeroes into the most significant bits vacated by the
  // shift. An ambiguous condition exists if u is greater than or
  // equal to the number of bits in a cell.
  OP(RSHIFT)
    tos = *sp-- >> tos;
  NEXT();

  // within ( n1|u1 n2|u2 n3|u3 -- flag )
  // Perform a comparison of a test value n1|u 1 with a lower limit
  // n2|u2 and an upper limit n3|u3, returning true if either (n2|u2 <
  // n3|u3 and (n2|u2 <= n1|u1 and n1|u1 < n3|u3)) or (n2|u2 > n3|u3
  // and (n2|u2 <= n1|u1 or n1|u1 < n3|u3)) is true, returning false
  // otherwise.
  OP(WITHIN)
#if 0
    tmp = *sp--;
    tos = ((*sp <= tos) & (*sp >= tmp)) ? -1 : 0;
    sp--;
  NEXT();
#else
  // : within ( n1|u1 n2|u2 n3|u3 -- flag ) >r over swap < swap r> > or not ;
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

  // abs ( n -- u )
  // u is the absolute value of n.
  OP(ABS)
#if 0
    if (tos < 0) tos = -tos;
  NEXT();
#elif 1
  // : abs ( n -- u ) dup 0< ?exit negate ;
  static const code_t ABS_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(ZERO_LESS),
    FVM_OP(ZERO_EXIT),
    FVM_OP(NEGATE),
    FVM_OP(EXIT)
  };
  CALL(ABS_CODE);
#else
  // : abs ( n -- u ) dup 0< swap over + xor ;
  static const code_t ABS_CODE[] PROGMEM = {
    FVM_OP(DUP),
    FVM_OP(ZERO_LESS),
    FVM_OP(SWAP),
    FVM_OP(OVER),
    FVM_OP(PLUS),
    FVM_OP(XOR),
    FVM_OP(EXIT)
  };
  CALL(ABS_CODE);
#endif

  // min ( n1 n2 -- n3 )
  // n3 is the lesser of n1 and n2.
  OP(MIN)
#if 0
    tmp = *sp--;
    if (tmp < tos) tos = tmp;
  NEXT();
#elif 0
  // : min ( n1 n2 -- n3 ) 2dup > if swap then drop ;
  static const code_t MIN_CODE[] PROGMEM = {
    FVM_OP(TWO_DUP),
    FVM_OP(GREATER),
    FVM_OP(ZERO_BRANCH), 2,
      FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(MIN_CODE);
#else
  // : min ( n1 n2 -- n3 ) over - dup 0< and + ;
  static const code_t MIN_CODE[] PROGMEM = {
    FVM_OP(OVER),
    FVM_OP(MINUS),
    FVM_OP(DUP),
    FVM_OP(ZERO_LESS),
    FVM_OP(AND),
    FVM_OP(PLUS),
    FVM_OP(EXIT)
  };
  CALL(MIN_CODE);
#endif

  // max ( n1 n2 -- n3 )
  // n3 is the greater of n1 and n2.
  OP(MAX)
#if 0
    tmp = *sp--;
    if (tmp > tos) tos = tmp;
  NEXT();
#elif 0
  // : max ( n1 n2 -- n3 ) 2dup < if swap then drop ;
  static const code_t MAX_CODE[] PROGMEM = {
    FVM_OP(TWO_DUP),
    FVM_OP(LESS),
    FVM_OP(ZERO_BRANCH), 2,
      FVM_OP(SWAP),
    FVM_OP(DROP),
    FVM_OP(EXIT)
  };
  CALL(MAX_CODE);
#else
  // : max ( n1 n2 -- n3 ) over swap - dup 0< and - ;
  static const code_t MAX_CODE[] PROGMEM = {
    FVM_OP(OVER),
    FVM_OP(SWAP),
    FVM_OP(MINUS),
    FVM_OP(DUP),
    FVM_OP(ZERO_LESS),
    FVM_OP(AND),
    FVM_OP(MINUS),
    FVM_OP(EXIT)
  };
  CALL(MAX_CODE);
#endif

  // bool ( x -- flag )
  // flag is true if and only if x is not equal to zero.
  OP(BOOL)
  FALLTHROUGH();

  // 0<> ( x -- flag )
  // flag is true if and only if x is not equal to zero.
  OP(ZERO_NOT_EQUALS)
#if 1
    tos = (tos != 0) ? -1 : 0;
  NEXT();
#else
  // : 0<> ( x -- flag ) 0= not ;
  static const code_t ZERO_NOT_EQUALS_CODE[] PROGMEM = {
    FVM_OP(ZERO_EQUALS),
    FVM_OP(NOT),
    FVM_OP(EXIT)
  };
  CALL(ZERO_NOT_EQUALS_CODE);
#endif

  // 0< ( n -- flag )
  // flag is true if and only if n is less than zero.
  OP(ZERO_LESS)
#if 1
    tos = (tos < 0) ? -1 : 0;
  NEXT();
#else
  // : 0< ( n -- flag ) 15 rshift ;
  static const code_t ZERO_LESS_CODE[] PROGMEM = {
    FVM_CLIT(15),
    FVM_OP(RSHIFT),
    FVM_OP(EXIT)
  };
  CALL(ZERO_LESS_CODE);
#endif

  // not ( x -- flag )
  // flag is true if and only if x is equal to zero.
  OP(NOT)
  FALLTHROUGH();

  // 0= ( x -- flag )
  // flag is true if and only if x is equal to zero.
  OP(ZERO_EQUALS)
    tos = (tos == 0) ? -1 : 0;
  NEXT();

  // 0> ( n -- flag )
  // flag is true if and only if n is greater than zero.
  OP(ZERO_GREATER)
    tos = (tos > 0) ? -1 : 0;
  NEXT();

  // <> ( x1 x2 -- flag )
  // flag is true if and only if x1 is not bit-for-bit the same as x2.
  OP(NOT_EQUALS)
#if 0
    tos = (*sp-- != tos) ? -1 : 0;
  NEXT();
#else
  // : <> ( x1 x2 -- flag ) - bool ;
  static const code_t NOT_EQUALS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(BOOL),
    FVM_OP(EXIT)
  };
  CALL(NOT_EQUALS_CODE);
#endif

  // < ( n1 n2 -- flag )
  // flag is true if and only if n1 is less than n2.
  OP(LESS)
#if 0
    tos = (*sp-- < tos) ? -1 : 0;
  NEXT();
#else
  // : < ( n1 n2 -- flag ) - 0< ;
  static const code_t LESS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_LESS),
    FVM_OP(EXIT)
  };
  CALL(LESS_CODE);
#endif

  // = ( x1 x2 -- flag )
  // flag is true if and only if x1 is bit-for-bit the same as x2.
  OP(EQUALS)
#if 0
    tos = (*sp-- == tos) ? -1 : 0;
  NEXT();
#else
  // : = ( x1 x2 -- flag ) - 0= ;
  static const code_t EQUALS_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_EQUALS),
    FVM_OP(EXIT)
  };
  CALL(EQUALS_CODE);
#endif

  // > ( n1 n2 -- flag )
  // flag is true if and only if n1 is greater than n2.
  OP(GREATER)
#if 0
    tos = (*sp-- > tos) ? -1 : 0;
  NEXT();
#else
  // : > ( n1 n2 -- flag ) - 0> ;
  static const code_t GREATER_CODE[] PROGMEM = {
    FVM_OP(MINUS),
    FVM_OP(ZERO_GREATER),
    FVM_OP(EXIT)
  };
  CALL(GREATER_CODE);
#endif

  // u< ( u1 u2 -- flag )
  // flag is true if and only if u1 is less than u2.
  OP(U_LESS)
    tos = ((ucell_t) *sp-- < (ucell_t) tos) ? -1 : 0;
  NEXT();

  // lookup ( str -- n )
  // Lookup string in dictionary.
  OP(LOOKUP)
    tos = lookup((const char*) tos);
  NEXT();

  // >body ( xt -- a-addr )
  // a-addr is the data-field address corresponding to xt. An
  // ambiguous condition exists if xt is not for a defined word.
  OP(TO_BODY)
    tp = (code_P) pgm_read_word(fntab+(tos-KERNEL_MAX));
    tos = fetch_word(tp+1);
  NEXT();

  // words ( -- )
  // Print words in dictionary.
  OP(WORDS)
#if 0
  {
    const char* s;
    int len;
    int nr = 0;
    for (int i = 0; (s = (const char*) pgm_read_word(&opstr[i])) != 0; i++) {
      len = ios.print((const __FlashStringHelper*) s);
      if (++nr % 5 == 0)
	ios.println();
      else {
	for (;len < 16; len++) ios.print(' ');
      }
    }
    for (int i = 0; (s = (const char*) pgm_read_word(&fnstr[i])) != 0; i++) {
      len = ios.print((const __FlashStringHelper*) s);
      if (++nr % 5 == 0)
	ios.println();
      else {
	for (;len < 16; len++) ios.print(' ');
      }
    }
  }
  NEXT();
#else
  // : words ( -- )
  //   0 begin
  //     begin
  //       dup .name ?dup
  //     while
  //       >r 1+ dup 5 mod
  //       if 16 r> - spaces
  //       else cr r> drop
  //       then
  //     repeat
  //     cr
  //     255 > if exit then
  //     256
  //   again ;
  static const code_t WORDS_CODE[] PROGMEM = {
    FVM_OP(ZERO),
      FVM_OP(DUP),
      FVM_OP(DOT_NAME),
      FVM_OP(QUESTION_DUP),
    FVM_OP(ZERO_BRANCH), 21,
      FVM_OP(TO_R),
      FVM_OP(ONE_PLUS),
      FVM_OP(DUP),
      FVM_CLIT(5),
      FVM_OP(MOD),
      FVM_OP(ZERO_BRANCH), 8,
        FVM_CLIT(16),
        FVM_OP(R_FROM),
        FVM_OP(MINUS),
        FVM_OP(SPACES),
      FVM_OP(BRANCH), -19,
        FVM_OP(CR),
        FVM_OP(R_FROM),
        FVM_OP(DROP),
    FVM_OP(BRANCH), -24,
    FVM_OP(CR),
    FVM_LIT(255),
    FVM_OP(GREATER),
    FVM_OP(ZERO_BRANCH), 2,
      FVM_OP(EXIT),
    FVM_CLIT(16),
    FVM_OP(SPACES),
    FVM_LIT(256),
    FVM_OP(BRANCH), -40
  };
  CALL(WORDS_CODE);
#endif

  // base ( -- a-addr )
  // a-addr is the address of a cell containing the current
  // number-conversion radix.
  OP(BASE)
    *++sp = tos;
    tos = (cell_t) &task.m_base;
  NEXT();

  // hex ( -- )
  // Set the numeric conversion radix to sixteen (hexa-decimal).
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
  // Set the numeric conversion radix to ten (decimal).
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

  // ?key ( -- c true | false )
  // Read character if available.
  OP(QUESTION_KEY)
    *++sp = tos;
    if (ios.available()) {
      *++sp = ios.read();
      tos = -1;
    }
    else {
      tos = 0;
    }
  NEXT();

  // key ( -- char ) begin ?key ?exit yield again ;
  // Receive one character char, a member of the implementation-
  // defined character set. Keyboard events that do not correspond to
  // such characters are discarded until a valid character is
  // received, and those events are subsequently unavailable. All
  // standard characters can be received. Characters
  // received are not displayed.
  OP(KEY)
  static const code_t KEY_CODE[] PROGMEM = {
      FVM_OP(QUESTION_KEY),
      FVM_OP(NOT),
      FVM_OP(ZERO_EXIT),
      FVM_OP(YIELD),
    FVM_OP(BRANCH), -5,
  };
  CALL(KEY_CODE);

  // emit ( x -- )
  // If x is a graphic character in the implementation-defined
  // character set, display x. The effect for all other values
  // of x is implementation-defined.
  OP(EMIT)
    ios.print((char) tos);
    tos = *sp--;
  NEXT();

  // cr ( -- )
  // Cause subsequent output to appear at the beginning of the next
  // line.
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
  // Display one space.
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
  // If n is greater than zero, display n spaces.
  OP(SPACES)
#if 0
    while (tos-- > 0) ios.print(' ');
    tos = *sp--;
  NEXT();
#else
  // : spaces ( n -- ) begin ?dup ?exit space 1- again ;
  static const code_t SPACES_CODE[] PROGMEM = {
      FVM_OP(QUESTION_DUP),
      FVM_OP(ZERO_EXIT),
      FVM_OP(SPACE),
      FVM_OP(ONE_MINUS),
    FVM_OP(BRANCH), -5,
  };
  CALL(SPACES_CODE);
#endif

  // u. ( u -- )
  // Display u in free field format.
  OP(U_DOT)
    ios.print((ucell_t) tos, task.m_base);
    tos = *sp--;
  NEXT();

  // . ( n -- )
  // Display n in free field format.
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
    FVM_OP(ZERO_BRANCH), 9,
      FVM_OP(DUP),
      FVM_OP(ZERO_LESS),
      FVM_OP(ZERO_BRANCH), 5,
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
  // Display stack contents.
  OP(DOT_S)
#if 0
    tmp = (sp - task.m_sp0);
    ios.print('[');
    ios.print(tmp);
    ios.print(F("]: "));
    if (tmp > 0) {
      cell_t* tp = task.m_sp0 + 1;
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
      FVM_OP(QUESTION_DUP),
    FVM_OP(ZERO_BRANCH), 7,
      FVM_OP(DUP),
      FVM_OP(PICK),
      FVM_OP(DOT),
      FVM_OP(ONE_MINUS),
    FVM_OP(BRANCH), -8,
    FVM_OP(CR),
    FVM_OP(EXIT)
  };
  CALL(DOT_S_CODE);
#endif

  // ." string" ( -- )
  // Display literal data or program memory string.
  OP(DOT_QUOTE)
    if (ip < (code_P) CODE_P_MAX)
      ip += ios.print((const __FlashStringHelper*) ip) + 1;
    else
      ip += ios.print((const char*) ip - CODE_P_MAX) + 1;
  NEXT();

  // type ( a-addr -- )
  // Display data memory string.
  OP(TYPE)
    ios.print((const char*) tos);
    tos = *sp--;
  NEXT();

  // .name ( xt -- length | 0 )
  // Display name of word (token from lookup) and return length.
  OP(DOT_NAME)
  {
    const __FlashStringHelper* s = NULL;
    if (tos < KERNEL_MAX)
      s = (const __FlashStringHelper*) pgm_read_word(&opstr[tos]);
    else if (tos < APPLICATION_MAX)
      s = (const __FlashStringHelper*) pgm_read_word(&fnstr[tos-KERNEL_MAX]);
    tos = (s != NULL) ? ios.print(s) : 0;
  }
  NEXT();

  // ? ( a-addr -- ) @ . ;
  // Display value of cell at a-addr.
  OP(QUESTION)
  static const code_t QUESTION_CODE[] PROGMEM = {
    FVM_OP(FETCH),
    FVM_OP(DOT),
    FVM_OP(EXIT)
  };
  CALL(QUESTION_CODE);

  // delay ( ms -- )
  // Yield while waiting given number of milli-seconds.
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
    FVM_OP(ZERO_BRANCH), 4,
      FVM_OP(YIELD),
    FVM_OP(BRANCH), -9,
    FVM_OP(R_FROM),
    FVM_OP(TWO_DROP),
    FVM_OP(EXIT)
  };
  CALL(DELAY_CODE);

  // micros ( -- us )
  // Micro-seconds.
  OP(MICROS)
    *++sp = tos;
    tos = micros();
  NEXT();

  // millis ( -- ms )
  // Milli-seconds.
  OP(MILLIS)
    *++sp = tos;
    tos = millis();
  NEXT();

  // pinmode ( mode pin -- )
  // Set digital pin mode.
  OP(PINMODE)
    pinMode(tos, *sp--);
    tos = *sp--;
  NEXT();

  // digitalread ( pin -- state )
  // Read digital pin.
  OP(DIGITALREAD)
    tos = digitalRead(tos);
  NEXT();

  // digitalwrite ( state pin -- )
  // Write digital pin.
  OP(DIGITALWRITE)
    digitalWrite(tos, *sp--);
    tos = *sp--;
  NEXT();

  // digitaltoggle ( pin -- )
  // Toggle digital pin.
  OP(DIGITALTOGGLE)
    digitalWrite(tos, !digitalRead(tos));
    tos = *sp--;
  NEXT();

  // analogread ( pin -- sample )
  // Read analog pin.
  OP(ANALOGREAD)
    tos = analogRead(tos & 0xf);
  NEXT();

  // analogwrite ( n pin -- )
  // Write pwm pin.
  OP(ANALOGWRITE)
    analogWrite(tos, *sp--);
    tos = *sp--;
  NEXT();

  // fncall ( -- )
  // Internal threaded code call.
  FNCALL:
#if (FVM_KERNEL_OPT == 1)
    if (fetch_byte(ip)) *++rp = ip;
#else
    *++rp = ip;
#endif
    ip = tp;
  NEXT();

  default:
    ;
  }
  return (-1);
}

int FVM::execute(int op, task_t& task)
{
  if (op < 0 || op > TOKEN_MAX) return (-1);
  static const code_t EXECUTE_CODE[] PROGMEM = {
    FVM_OP(EXECUTE),
    FVM_OP(HALT)
  };
  task.push(op);
  return (execute(EXECUTE_CODE, task));
}

int FVM::interpret(task_t& task)
{
  char buffer[32];
  char c = scan(buffer, task);
  int res = execute(buffer, task);
  if (res == 1) {
    while ((res = resume(task)) > 0);
  }
  else if (res == -1) {
    char* endptr;
    int value = strtol(buffer, &endptr, task.m_base);
    if (*endptr != 0) {
      task.m_ios.print(buffer);
      task.m_ios.println(F(" ??"));
      return (res);
    }
    task.push(value);
    res = execute(OP_NOOP, task);
  }
  if (c == '\n' && !task.trace())
    execute(FVM::OP_DOT_S, task);
  return (res);
}

#if (FVM_KERNEL_DICT == 1)
static const char EXIT_PSTR[] PROGMEM = "exit";
static const char ZERO_EXIT_PSTR[] PROGMEM = "?exit";
static const char LIT_PSTR[] PROGMEM = "(lit)";
static const char CLIT_PSTR[] PROGMEM = "(clit)";
static const char SLIT_PSTR[] PROGMEM = "(slit)";
static const char VAR_PSTR[] PROGMEM = "(var)";
static const char CONST_PSTR[] PROGMEM = "(const)";
static const char FUNC_PSTR[] PROGMEM = "(func)";
static const char DOES_PSTR[] PROGMEM = "(does)";
static const char PARAM_PSTR[] PROGMEM = "(param)";
static const char BRANCH_PSTR[] PROGMEM = "(branch)";
static const char ZERO_BRANCH_PSTR[] PROGMEM = "(0branch)";
static const char DO_PSTR[] PROGMEM = "(do)";
static const char I_PSTR[] PROGMEM = "i";
static const char J_PSTR[] PROGMEM = "j";
static const char LEAVE_PSTR[] PROGMEM = "leave";
static const char LOOP_PSTR[] PROGMEM = "(loop)";
static const char PLUS_LOOP_PSTR[] PROGMEM = "(+loop)";
static const char NOOP_PSTR[] PROGMEM = "noop";
static const char EXECUTE_PSTR[] PROGMEM = "execute";
static const char YIELD_PSTR[] PROGMEM = "yield";
static const char HALT_PSTR[] PROGMEM = "halt";
static const char KERNEL_PSTR[] PROGMEM = "(kernel)";
static const char CALL_PSTR[] PROGMEM = "(call)";
static const char TRACE_PSTR[] PROGMEM = "trace";
static const char ROOM_PSTR[] PROGMEM = "room";

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
static const char COMPILE_PSTR[] PROGMEM = "(compile)";

static const char TO_R_PSTR[] PROGMEM = ">r";
static const char R_FROM_PSTR[] PROGMEM = "r>";
static const char R_FETCH_PSTR[] PROGMEM = "r@";

static const char SP_PSTR[] PROGMEM = "sp";
static const char DEPTH_PSTR[] PROGMEM = "depth";
static const char DROP_PSTR[] PROGMEM = "drop";
static const char NIP_PSTR[] PROGMEM = "nip";
static const char EMPTY_PSTR[] PROGMEM = "empty";
static const char DUP_PSTR[] PROGMEM = "dup";
static const char QUESTION_DUP_PSTR[] PROGMEM = "?dup";
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

static const char MINUS_TWO_PSTR[] PROGMEM = "-2";
static const char MINUS_ONE_PSTR[] PROGMEM = "-1";
static const char ZERO_PSTR[] PROGMEM = "0";
static const char ONE_PSTR[] PROGMEM = "1";
static const char TWO_PSTR[] PROGMEM = "2";
static const char CELL_PSTR[] PROGMEM = "cell";
static const char CELLS_PSTR[] PROGMEM = "cells";

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


static const char WITHIN_PSTR[] PROGMEM = "within";
static const char ABS_PSTR[] PROGMEM = "abs";
static const char MIN_PSTR[] PROGMEM = "min";
static const char MAX_PSTR[] PROGMEM = "max";

static const char ZERO_NOT_EQUALS_PSTR[] PROGMEM = "0<>";
static const char ZERO_LESS_PSTR[] PROGMEM = "0<";
static const char ZERO_EQUALS_PSTR[] PROGMEM = "0=";
static const char ZERO_GREATER_PSTR[] PROGMEM = "0>";
static const char NOT_EQUALS_PSTR[] PROGMEM = "<>";
static const char LESS_PSTR[] PROGMEM = "<";
static const char EQUALS_PSTR[] PROGMEM = "=";
static const char GREATER_PSTR[] PROGMEM = ">";
static const char U_LESS_PSTR[] PROGMEM = "u<";

static const char LOOKUP_PSTR[] PROGMEM = "lookup";
static const char TO_BODY_PSTR[] PROGMEM = ">body";
static const char WORDS_PSTR[] PROGMEM = "words";

static const char BASE_PSTR[] PROGMEM = "base";
static const char HEX_PSTR[] PROGMEM = "hex";
static const char DECIMAL_PSTR[] PROGMEM = "decimal";
static const char QUESTION_KEY_PSTR[] PROGMEM = "?key";
static const char KEY_PSTR[] PROGMEM = "key";
static const char EMIT_PSTR[] PROGMEM = "emit";
static const char CR_PSTR[] PROGMEM = "cr";
static const char SPACE_PSTR[] PROGMEM = "space";
static const char SPACES_PSTR[] PROGMEM = "spaces";
static const char U_DOT_PSTR[] PROGMEM = "u.";
static const char DOT_PSTR[] PROGMEM = ".";
static const char DOT_S_PSTR[] PROGMEM = ".s";
static const char DOT_QUOTE_PSTR[] PROGMEM = "(.\")";
static const char TYPE_PSTR[] PROGMEM = "type";
static const char DOT_NAME_PSTR[] PROGMEM = ".name";
static const char QUESTION_PSTR[] PROGMEM = "?";

static const char MICROS_PSTR[] PROGMEM = "micros";
static const char MILLIS_PSTR[] PROGMEM = "millis";
static const char DELAY_PSTR[] PROGMEM = "delay";
static const char PINMODE_PSTR[] PROGMEM = "pinmode";
static const char DIGITALREAD_PSTR[] PROGMEM = "digitalread";
static const char DIGITALWRITE_PSTR[] PROGMEM = "digitalwrite";
static const char DIGITALTOGGLE_PSTR[] PROGMEM = "digitaltoggle";
static const char ANALOGREAD_PSTR[] PROGMEM = "analogread";
static const char ANALOGWRITE_PSTR[] PROGMEM = "analogwrite";
#endif

const str_P FVM::opstr[] PROGMEM = {
#if (FVM_KERNEL_DICT == 1)
  (str_P) EXIT_PSTR,
  (str_P) ZERO_EXIT_PSTR,
  (str_P) LIT_PSTR,
  (str_P) CLIT_PSTR,
  (str_P) SLIT_PSTR,
  (str_P) VAR_PSTR,
  (str_P) CONST_PSTR,
  (str_P) FUNC_PSTR,
  (str_P) DOES_PSTR,
  (str_P) PARAM_PSTR,
  (str_P) BRANCH_PSTR,
  (str_P) ZERO_BRANCH_PSTR,
  (str_P) DO_PSTR,
  (str_P) I_PSTR,
  (str_P) J_PSTR,
  (str_P) LEAVE_PSTR,
  (str_P) LOOP_PSTR,
  (str_P) PLUS_LOOP_PSTR,
  (str_P) NOOP_PSTR,
  (str_P) EXECUTE_PSTR,
  (str_P) HALT_PSTR,
  (str_P) YIELD_PSTR,
  (str_P) KERNEL_PSTR,
  (str_P) CALL_PSTR,
  (str_P) TRACE_PSTR,
  (str_P) ROOM_PSTR,

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
  (str_P) COMPILE_PSTR,

  (str_P) TO_R_PSTR,
  (str_P) R_FROM_PSTR,
  (str_P) R_FETCH_PSTR,

  (str_P) SP_PSTR,
  (str_P) DEPTH_PSTR,
  (str_P) DROP_PSTR,
  (str_P) NIP_PSTR,
  (str_P) EMPTY_PSTR,
  (str_P) DUP_PSTR,
  (str_P) QUESTION_DUP_PSTR,
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

  (str_P) MINUS_TWO_PSTR,
  (str_P) MINUS_ONE_PSTR,
  (str_P) ZERO_PSTR,
  (str_P) ONE_PSTR,
  (str_P) TWO_PSTR,
  (str_P) CELL_PSTR,
  (str_P) CELLS_PSTR,

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
  (str_P) QUESTION_KEY_PSTR,
  (str_P) KEY_PSTR,
  (str_P) EMIT_PSTR,
  (str_P) CR_PSTR,
  (str_P) SPACE_PSTR,
  (str_P) SPACES_PSTR,
  (str_P) U_DOT_PSTR,
  (str_P) DOT_PSTR,
  (str_P) DOT_S_PSTR,
  (str_P) DOT_QUOTE_PSTR,
  (str_P) TYPE_PSTR,
  (str_P) DOT_NAME_PSTR,
  (str_P) QUESTION_PSTR,

  (str_P) MICROS_PSTR,
  (str_P) MILLIS_PSTR,
  (str_P) DELAY_PSTR,
  (str_P) PINMODE_PSTR,
  (str_P) DIGITALREAD_PSTR,
  (str_P) DIGITALWRITE_PSTR,
  (str_P) DIGITALTOGGLE_PSTR,
  (str_P) ANALOGREAD_PSTR,
  (str_P) ANALOGWRITE_PSTR,
#endif
  0
};
