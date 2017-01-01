/**
 * @file FVM.h
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

#ifndef ARDUINO_FVM_H
#define ARDUINO_FVM_H

#include <Arduino.h>

/**
 * Enable symbolic trace of virtual machine instruction cycle. Print
 * execute time, instruction pointer, operation code and stack
 * contents.
 */
#define FVM_TRACE

/**
 * Enable kernel dictionary. Remove to reduce foot-print for
 * non-interactive application.
 */
#define FVM_DICT

/**
 * String in program memory.
 */
typedef const PROGMEM char* str_P;

class FVM {
 public:
  enum {
    /*
     * Control structure and literals
     */
    OP_EXIT,			// Function return
    OP_MINUS_EXIT,		// Function return if zero/false
    OP_LIT,			// Inline literal constant
    OP_CLIT,		 	// Inline literal signed character constant
    OP_SLIT,		 	// Push instruction pointer and branch always
    OP_VAR,			// Handle variable reference
    OP_CONST,			// Handle constant
    OP_DOES,			// Handle object pointer
    OP_PARAM,			// Duplicate inline index stack element
    OP_BRANCH,			// Branch always
    OP_ZERO_BRANCH,		// Branch if zero/false
    OP_COMPILE,			// Add inline operation/function code
    OP_EXECUTE,			// Execute operation or function
    OP_TRACE,			// Set trace mode

    /*
     * Memory access
     */
    OP_C_FETCH,			// Load character (signed byte)
    OP_C_STORE,			// Store character
    OP_FETCH,			// Load data
    OP_STORE,			// Store data
    OP_PLUS_STORE,		// Update data
    OP_DP,			// Data pointer variable
    OP_HERE,			// Data pointer
    OP_ALLOT,			// Allocate number of bytes
    OP_COMMA,			// Allocate and assign from top of stack
    OP_C_COMMA,			// Allocate and assign character
    OP_CELLS,			// Convert cells to bytes for allot

    /*
     * Return stack
     */
    OP_TO_R,			// Push data on return stack
    OP_R_FROM,			// Pop data from return stack
    OP_R_FETCH,			// Copy from return stack
    OP_QR,			// Decrement top of return stack

    /*
     * Parameter stack
     */
    OP_SP,			// Stack pointer
    OP_DEPTH,			// Number of elements
    OP_DROP,			// Drop top of stack
    OP_NIP,			// Drop next top of stack
    OP_EMPTY,			// Empty stack
    OP_DUP,			// Duplicate top of stack
    OP_QDUP,			// Duplicate top of stack if not zero
    OP_OVER,			// Duplicate next top of stack
    OP_TUCK,			// Duplicate top of stack and rotate
    OP_PICK,			// Duplicate index stack element
    OP_SWAP,			// Swap two top stack elements
    OP_ROT,			// Rotate three top stack elements
    OP_MINUS_ROT,		// Inverse rotate three top stack elements
    OP_ROLL,			// Rotate given number of stack elements
    OP_TWO_SWAP,		// Swap two double stack elements
    OP_TWO_DUP,			// Duplicate double stack elements
    OP_TWO_OVER,		// Duplicate double next top of stack
    OP_TWO_DROP,		// Drop double top of stack

    /*
     * Constants
     */
    OP_CELL,			// Stack width in bytes
    OP_MINUS_TWO,		// Push constant(-2)
    OP_MINUS_ONE,		// Push constant(-1)
    OP_ZERO,			// Push constant(0)
    OP_ONE,			// Push constant(1)
    OP_TWO,			// Push constant(2)

    /*
     * Bitwise logical operations
     */
    OP_BOOL,			// Convert top of stack to boolean
    OP_NOT,			// Convert top of stack to invert boolean
    OP_TRUE,			// Push true(-1)
    OP_FALSE,			// Push false(0)
    OP_INVERT,			// Bitwise inverse top element
    OP_AND,			// Bitwise AND top two elements
    OP_OR,			// Bitwise OR top two elements
    OP_XOR,			// Bitwise XOR top two elements

    /*
     * Arithmetic operations
     */
    OP_NEGATE,			// Negate top of stack
    OP_ONE_PLUS,		// Increment top of stack
    OP_ONE_MINUS,		// Decrement top of stack
    OP_TWO_PLUS,		// Increment by two
    OP_TWO_MINUS,		// Decrement by two
    OP_TWO_STAR,		// Multiply by two
    OP_TWO_SLASH,		// Divide by two
    OP_PLUS,			// Add top two elements
    OP_MINUS,			// Substract top two elements
    OP_STAR,			// Multiply top two elements
    OP_STAR_SLASH,		// Multiply/Divide top three elements
    OP_SLASH,			// Quotient for division of top two elements
    OP_MOD,			// Remainder for division of top two elements
    OP_SLASH_MOD,		// Quotient and remainder
    OP_LSHIFT,			// Left shift
    OP_RSHIFT,			// Right shift

    /*
     * Math operations
     */
    OP_WITHIN,			// Within boundard
    OP_ABS,			// Absolute value
    OP_MIN,			// Minimum value
    OP_MAX,			// Maximum value

    /*
     * Relational operations
     */
    OP_ZERO_NOT_EQUALS,		// Not equal zero
    OP_ZERO_LESS,		// Less than zero
    OP_ZERO_EQUALS,		// Equal to zero
    OP_ZERO_GREATER,		// Greater than zero
    OP_NOT_EQUALS,		// Not equal
    OP_LESS,			// Less than
    OP_EQUALS,			// Equal
    OP_GREATER,			// Greater than
    OP_U_LESS,			// Unsigned less than

    /*
     * Dictionary functions
     */
    OP_LOOKUP,			// Lookup word in dictionary
    OP_TO_BODY,			// Access data area application variable
    OP_WORDS,			// Print list of operations/functions

    /*
     * Basic I/O
     */
    OP_BASE,			// Base for number conversion
    OP_HEX,			// Set hexa-decimal number conversion base
    OP_DECIMAL,			// Set decimal number conversion base
    OP_QKEY,			// Read character if available
    OP_KEY,			// Wait for character and read
    OP_EMIT,			// Print character
    OP_CR,			// Print new-line
    OP_SPACE,			// Print space
    OP_SPACES,			// Print spaces
    OP_U_DOT,			// Print top of stack as unsigned
    OP_DOT,			// Print top of stack
    OP_DOT_S,			// Print contents of parameter stack
    OP_DOT_NAME,		// Print operation/function name
    OP_Q,			// Print value of variable

    /*
     * Arduino extensions
     */
    OP_MICROS,			// Micro-seconds
    OP_MILLIS,			// Milli-seconds
    OP_DELAY,			// Delay milli-seconds (yield)
    OP_PINMODE,			// Digital pin mode
    OP_DIGITALREAD,		// Read digital pin
    OP_DIGITALWRITE,		// Write digital pin
    OP_DIGITALTOGGLE,		// Toggle digital pin
    OP_ANALOGREAD,		// Read analog pin
    OP_ANALOGWRITE,		// Write pwm pin

    /*
     * High-level control
     */
    OP_HALT,			// Halt virtual machine
    OP_YIELD,			// Yield virtual machine

    /*
     * Last operation code
     */
    OP_NOP			// No-operation
  } __attribute__((packed));

  typedef int16_t data_t;
  typedef uint16_t udata_t;
  typedef int32_t data2_t;
  typedef uint32_t udata2_t;
  typedef int8_t code_t;
  typedef const PROGMEM code_t* code_P;

  struct const_t {
    code_t op;
    data_t value;
  };

  struct var_t {
    code_t op;
    data_t* value;
  };

  struct task_t {
    // Default stack sizes
    static const int SP0_MAX = 32;
    static const int RP0_MAX = 16;

    code_P m_ip;		// Instruction pointer
    code_P* m_rp;		// Return stack pointer
    data_t* m_sp;		// Parameter stack pointer
    Stream& m_ios;		// Input/Output stream
    data_t m_base;		// Number conversion base
    bool m_trace;		// Trace mode

    data_t m_sp0[SP0_MAX];	// Parameter stack
    code_P m_rp0[RP0_MAX];	// Return stack

    /**
     * Construct task with given in-/output stream and function
     * pointer.
     * @param[in] ios in-/output stream.
     * @param[in] fn function pointer (default none).
     */
    task_t(Stream& ios, code_P fn = 0) :
      m_ip(fn),
      m_rp(m_rp0),
      m_sp(m_sp0 + 1),
      m_ios(ios),
      m_base(10),
      m_trace(false)
    {}

    /**
     * Push value to parameter stack.
     * @param[in] value to push.
     */
    void push(data_t value)
    {
      *++m_sp = value;
    }

    /**
     * Pop value from parameter stack.
     * @return value.
     */
    data_t pop()
    {
      return (*m_sp--);
    }

    /**
     * Current trace mode.
     * @return trace mode.
     */
    bool trace()
    {
      return (m_trace);
    }

    /**
     * Enable/disable trace mode.
     * @param[in] flag trace mode.
     */
    void trace(bool flag)
    {
      m_trace = flag;
    }

    /**
     * Set task instruction pointer to given function pointer.
     * @param[in] fn function pointer.
     * @return task reference.
     */
    task_t& call(code_P fn)
    {
      m_ip = fn;
      return (*this);
    }
  };

  /**
   * Construct forth virtual machine with given data area.
   * @param[in] dp0 initial data pointer.
   */
  FVM(void* dp0 = 0) : m_dp((uint8_t*) dp0) {}

  /**
   * Lookup given string in dictionary. Return operation code (0..127)
   * or function index (128..255), otherwise negative error code(-1).
   * @param[in] name string.
   * @return error code.
   */
  int lookup(const char* name);

  /**
   * Resume task in virtual machine with given task. Returns on
   * yield(1), halt(0) or illegal instruction (-1).
   * @param[in] task to resume.
   * @return error code.
   */
  int resume(task_t& task);

  /**
   * Execute given operation (or function) with given task.
   * @param[in] op operation code or function index.
   * @param[in] task to resume.
   * @return error code.
   */
  int execute(code_t op, task_t& task);

  /**
   * Execute given function code pointer with task. Returns
   * on yield(1), halt(0) or illegal instruction (-1).
   * @param[in] fn function code pointer.
   * @param[in] task to resume.
   * @return error code.
   */
  int execute(code_P fn, task_t& task);

  /**
   * Lookup given name in dictionary and execute with given task.
   * Convert to number if possible, and push on task stack.
   * @param[in] name of function to execute.
   * @param[in] task to resume.
   * @return error code.
   */
  int execute(const char* name, task_t& task);

  /**
   * Current data pointer.
   */
  uint8_t* dp()
  {
    return (m_dp);
  }

  /**
   * Set data pointer.
   */
  void dp(void* dp)
  {
    m_dp = (uint8_t*) dp;
  }

  void compile(code_t op)
  {
    *m_dp++ = op;
  }

  // Function code and dictionary provided by sketch
  static const code_P fntab[] PROGMEM;
  static const str_P fnstr[] PROGMEM;

 protected:
  // Kernel dictionary
  static const str_P opstr[] PROGMEM;

  // Data allocation pointer
  uint8_t* m_dp;
};

/**
 * Compile virtual machine instruction.
 * @param[in] code operation code.
 */
#define FVM_OP(code) FVM::OP_ ## code

/**
 * Compile literal number.
 * @param[in] n number.
 */
#define FVM_LIT(n)							\
  FVM::OP_LIT,								\
  FVM::code_t((n) >> 8),						\
  FVM::code_t(n)

/**
 * Compile literal number/character.
 * @param[in] n number.
 */
#define FVM_CLIT(n)							\
  FVM::OP_CLIT,								\
  FVM::code_t(n)

/**
 * Compile call to given function in function table.
 * @param[in] fn function index in table.
 */
#define FVM_CALL(fn) FVM::code_t(-fn-1)

/**
 * Create a named reference to a created object.
 * @param[in] n name of object.
 * @param[in] fn action function (does).
 */
#define FVM_CREATE(n,does)						\
  const char n ## _PSTR[] PROGMEM = #n;					\
  const FVM::var_t n ## _VAR PROGMEM = { FVM_CALL(does), (FVM::data_t*) &n }

/**
 * Create a named reference to a variable.
 * @param[in] n name of variable.
 */
#define FVM_VARIABLE(n)							\
  const char n ## _PSTR[] PROGMEM = #n;					\
  const FVM::var_t n ## _VAR PROGMEM = { FVM_OP(VAR), (FVM::data_t*) &n }

/**
 * Create a constant value.
 * @param[in] n name of constant.
 */
#define FVM_CONSTANT(n,val)						\
  const char n ## _PSTR[] PROGMEM = #n;					\
  const FVM::const_t n ## _CONST PROGMEM = { FVM_OP(CONST), val }

#endif
