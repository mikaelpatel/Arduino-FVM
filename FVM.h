/**
 * @file FVM.h
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
    OP_EXIT = 0,	 	// Function return
    OP_MINUS_EXIT = 1,		// Function return if zero/false
    OP_LIT = 2,			// Inline literal constant
    OP_CLIT = 3,	 	// Inline literal signed character constant
    OP_SLIT = 4,	 	// Push instruction pointer and branch always
    OP_VAR = 5,			// Handle variable reference
    OP_CONST = 6,		// Handle constant
    OP_FUNC = 7,		// Handle function call
    OP_DOES = 8,		// Handle object pointer
    OP_PARAM = 9,		// Duplicate inline index stack element
    OP_BRANCH = 10,		// Branch always
    OP_ZERO_BRANCH = 11,	// Branch if zero/false
    OP_DO = 12,			// Start loop block
    OP_I = 13,			// Current loop index
    OP_J = 14,			// Outer loop index
    OP_LEAVE = 15,		// Mark loop block as completed
    OP_LOOP = 16,		// End loop block (one increment)
    OP_PLUS_LOOP = 17,		// End loop block (n increment)
    OP_COMPILE = 18,		// Add inline operation/function code
    OP_TRAP = 19,		// Extended instruction
    OP_EXECUTE = 20,		// Execute operation or function
    OP_TRACE = 21,		// Set trace mode

    /*
     * Memory access
     */
    OP_C_FETCH = 22,		// Load character (signed byte)
    OP_C_STORE = 23,		// Store character
    OP_FETCH = 24,		// Load data
    OP_STORE = 25,		// Store data
    OP_PLUS_STORE = 26,		// Update data
    OP_DP = 27,			// Data pointer variable
    OP_HERE = 28,		// Data pointer
    OP_ALLOT = 29,		// Allocate number of bytes
    OP_COMMA = 30,		// Allocate and assign from top of stack
    OP_C_COMMA = 31,		// Allocate and assign character
    OP_CELLS = 32,		// Convert cells to bytes for allot

    /*
     * Return stack
     */
    OP_TO_R = 33,		// Push data on return stack
    OP_R_FROM = 34,		// Pop data from return stack
    OP_R_FETCH = 35,		// Copy from return stack

    /*
     * Parameter stack
     */
    OP_SP = 36,			// Stack pointer
    OP_DEPTH = 37,		// Number of elements
    OP_DROP = 38,		// Drop top of stack
    OP_NIP = 39,		// Drop next top of stack
    OP_EMPTY = 40,		// Empty stack
    OP_DUP = 41,		// Duplicate top of stack
    OP_QUESTION_DUP = 42,	// Duplicate top of stack if not zero
    OP_OVER = 43,		// Duplicate next top of stack
    OP_TUCK = 44,		// Duplicate top of stack and rotate
    OP_PICK = 45,		// Duplicate index stack element
    OP_SWAP = 46,		// Swap two top stack elements
    OP_ROT = 47,		// Rotate three top stack elements
    OP_MINUS_ROT = 48,		// Inverse rotate three top stack elements
    OP_ROLL = 49,		// Rotate given number of stack elements
    OP_TWO_SWAP = 50,		// Swap two double stack elements
    OP_TWO_DUP = 51,		// Duplicate double stack elements
    OP_TWO_OVER = 52,		// Duplicate double next top of stack
    OP_TWO_DROP = 53,		// Drop double top of stack

    /*
     * Constants
     */
    OP_CELL = 54,		// Stack width in bytes
    OP_MINUS_TWO = 55,		// Push constant(-2)
    OP_MINUS_ONE = 56,		// Push constant(-1)
    OP_ZERO = 57,		// Push constant(0)
    OP_ONE = 58,		// Push constant(1)
    OP_TWO = 59,		// Push constant(2)

    /*
     * Bitwise logical operations
     */
    OP_BOOL = 60,		// Convert top of stack to boolean
    OP_NOT = 61,		// Convert top of stack to invert boolean
    OP_TRUE = 62,		// Push true(-1)
    OP_FALSE = 63,		// Push false(0)
    OP_INVERT = 64,		// Bitwise inverse top element
    OP_AND = 65,		// Bitwise AND top two elements
    OP_OR = 66,			// Bitwise OR top two elements
    OP_XOR = 67,		// Bitwise XOR top two elements

    /*
     * Arithmetic operations
     */
    OP_NEGATE = 68,		// Negate top of stack
    OP_ONE_PLUS = 69,		// Increment top of stack
    OP_ONE_MINUS = 70,		// Decrement top of stack
    OP_TWO_PLUS = 71,		// Increment by two
    OP_TWO_MINUS = 72,		// Decrement by two
    OP_TWO_STAR = 73,		// Multiply by two
    OP_TWO_SLASH = 74,		// Divide by two
    OP_PLUS = 75,		// Add top two elements
    OP_MINUS = 76,		// Substract top two elements
    OP_STAR = 77,		// Multiply top two elements
    OP_STAR_SLASH = 78,		// Multiply/Divide top three elements
    OP_SLASH = 79,		// Quotient for division of top two elements
    OP_MOD = 80,		// Remainder for division of top two elements
    OP_SLASH_MOD = 81,		// Quotient and remainder
    OP_LSHIFT = 82,		// Left shift
    OP_RSHIFT = 83,			// Right shift

    /*
     * Math operations
     */
    OP_WITHIN = 84,		// Within boundard
    OP_ABS = 85,		// Absolute value
    OP_MIN = 86,		// Minimum value
    OP_MAX = 87,		// Maximum value

    /*
     * Relational operations
     */
    OP_ZERO_NOT_EQUALS = 88,	// Not equal zero
    OP_ZERO_LESS = 89,		// Less than zero
    OP_ZERO_EQUALS = 90,	// Equal to zero
    OP_ZERO_GREATER = 91,	// Greater than zero
    OP_NOT_EQUALS = 92,		// Not equal
    OP_LESS = 93,		// Less than
    OP_EQUALS = 94,		// Equal
    OP_GREATER = 95,		// Greater than
    OP_U_LESS = 96,		// Unsigned less than

    /*
     * Dictionary functions
     */
    OP_LOOKUP = 97,		// Lookup word in dictionary
    OP_TO_BODY = 98,		// Access data area application variable
    OP_WORDS = 99,		// Print list of operations/functions

    /*
     * Basic I/O
     */
    OP_BASE = 100,		// Base for number conversion
    OP_HEX = 101,		// Set hexa-decimal number conversion base
    OP_DECIMAL = 102,		// Set decimal number conversion base
    OP_QUESTION_KEY = 103,	// Read character if available
    OP_KEY = 104,		// Wait for character and read
    OP_EMIT = 105,		// Print character
    OP_CR = 106,		// Print new-line
    OP_SPACE = 107,		// Print space
    OP_SPACES = 108,		// Print spaces
    OP_U_DOT = 109,		// Print top of stack as unsigned
    OP_DOT = 110,		// Print top of stack
    OP_DOT_S = 111,		// Print contents of parameter stack
    OP_DOT_QUOTE = 112,		// Print program memory string
    OP_TYPE = 113,		// Print string
    OP_DOT_NAME = 114,		// Print operation/function name
    OP_QUESTION = 115,		// Print value of variable

    /*
     * Arduino extensions
     */
    OP_MICROS = 116,		// Micro-seconds
    OP_MILLIS = 117,		// Milli-seconds
    OP_DELAY = 118,		// Delay milli-seconds (yield)
    OP_PINMODE = 119,		// Digital pin mode
    OP_DIGITALREAD = 120,	// Read digital pin
    OP_DIGITALWRITE = 121,	// Write digital pin
    OP_DIGITALTOGGLE = 122,	// Toggle digital pin
    OP_ANALOGREAD = 123,	// Read analog pin
    OP_ANALOGWRITE = 124,	// Write pwm pin

    /*
     * High-level control
     */
    OP_HALT = 125,		// Halt virtual machine
    OP_YIELD = 126,		// Yield virtual machine

    /*
     * Last operation code
     */
    OP_NOP = 127		// No-operation
  } __attribute__((packed));

  typedef int16_t data_t;
  typedef uint16_t udata_t;
  typedef int32_t data2_t;
  typedef uint32_t udata2_t;
  typedef int8_t code_t;
  typedef const PROGMEM code_t* code_P;

  struct var_t {
    code_t op;
    data_t* value;
  };

  struct const_t {
    code_t op;
    data_t value;
  };

  typedef data_t* (*fn_t)(data_t* sp);
  struct func_t {
    code_t op;
    fn_t fn;
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
   * Get current data allocation pointer.
   * @return pointer.
   */
  uint8_t* dp()
  {
    return (m_dp);
  }

  /**
   * Set data allocation pointer.
   * @param[in] s string.
   */
  void dp(void* dp)
  {
    m_dp = (uint8_t*) dp;
  }

  /**
   * Allocate and copy given string to data area.
   * @param[in] s string.
   */
  void compile(const char* s)
  {
    strcpy((char*) m_dp, s);
    m_dp += strlen(s) + 1;
  }

  /**
   * Allocate and copy given operation code to data area.
   * @param[in] op operation code.
   */
  void compile(code_t op)
  {
    *m_dp++ = op;
  }

  /**
   * Lookup given string in dictionary. Return operation code (0..127)
   * or function index (128..255), otherwise negative error code(-1).
   * @param[in] name string.
   * @return error code.
   */
  int lookup(const char* name);

  /**
   * Scan token to given buffer. Return break character or negative
   * error code.
   * @param[in] bp buffer pointer.
   * @param[in] task stream to use.
   * @return break character or negative error code.
   */
  int scan(char* bp, task_t& task);

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
 * Compile literal number (little endian).
 * @param[in] n number.
 */
#define FVM_LIT(n)							\
  FVM::OP_LIT,								\
  FVM::code_t(n),							\
  FVM::code_t((n) >> 8)

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
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] does object handler.
 * @param[in] data storage.
 */
#define FVM_CREATE(id,var,does,data)					\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = #data;				\
  const FVM::var_t var ## _VAR PROGMEM = {				\
    FVM_CALL(does),							\
    (FVM::data_t*) &data						\
  }

/**
 * Create a named reference to a variable.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] data storage.
 */
#define FVM_VARIABLE(id,var,data)					\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = #data;				\
  const FVM::var_t var ## _VAR PROGMEM = {				\
    FVM_OP(VAR),							\
    (FVM::data_t*) &data						\
  }

/**
 * Create a constant value.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] name dictionary string.
 * @param[in] val value of constant.
 */
#define FVM_CONSTANT(id,var,name,val)					\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = name;				\
  const FVM::const_t var ## _CONST PROGMEM = {				\
    FVM_OP(CONST),							\
    val									\
  }

/**
 * Create a colon definition.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] name dictionary string.
 */
#define FVM_COLON(id,var,name)						\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = name;				\
  const FVM::code_t var ## _CODE[] PROGMEM = {

/**
 * Create an extension function handler.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] fn name of function.
 */
#define FVM_FUNCTION(id,var,fn)						\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = #fn;				\
  const FVM::func_t var ## _FUNC PROGMEM = {				\
    FVM_OP(FUNC),							\
    fn									\
  }

/**
 * Create a name table symbol.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] name dictionary string.
 */
#define FVM_SYMBOL(id,var,name)						\
  const int var = id + 128;						\
  const char var ## _PSTR[] PROGMEM = name

#endif
