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
    OP_ZERO_EXIT = 1,		// Function return if zero/false
    OP_LIT = 2,			// Inline literal constant
    OP_CLIT = 3,	 	// Inline literal signed character constant
    OP_SLIT = 4,	 	// Push instruction pointer and branch always
    OP_VAR = 5,			// Handle variable reference
    OP_CONST = 6,		// Handle constant
    OP_FUNC = 7,		// Handle function call
    OP_DOES = 8,		// Handle object pointer
    OP_PARAM = 9,		// Duplicate inline indexed stack element
    OP_BRANCH = 10,		// Branch always (offset -128..127)
    OP_ZERO_BRANCH = 11,	// Branch if zero/false (offset -128..127)
    OP_DO = 12,			// Start loop block
    OP_I = 13,			// Current loop index
    OP_J = 14,			// Outer loop index
    OP_LEAVE = 15,		// Mark loop block as completed
    OP_LOOP = 16,		// End loop block (one increment)
    OP_PLUS_LOOP = 17,		// End loop block (n increment)
    OP_EXECUTE = 18,		// Execute operation token
    OP_TAIL = 19,		// Tail call
    OP_HALT = 20,		// Halt virtual machine
    OP_YIELD = 21,		// Yield virtual machine
    OP_NOOP = 22,		// No operation
    OP_KERNEL = 23,		// Call inline kernel token
    OP_CALL = 24,		// Call application token
    OP_TRACE = 25,		// Set trace mode

    /*
     * Memory access
     */
    OP_C_FETCH = 26,		// Load character (signed byte)
    OP_C_STORE = 27,		// Store character
    OP_FETCH = 28,		// Load data
    OP_STORE = 29,		// Store data
    OP_PLUS_STORE = 30,		// Update data
    OP_DP = 31,			// Data pointer variable
    OP_HERE = 32,		// Data pointer
    OP_ALLOT = 33,		// Allocate number of bytes
    OP_COMMA = 34,		// Allocate and assign from top of stack
    OP_C_COMMA = 35,		// Allocate and assign character
    OP_COMPILE = 36,		// Add inline operation/function code

    /*
     * Return stack
     */
    OP_TO_R = 37,		// Push data on return stack
    OP_R_FROM = 38,		// Pop data from return stack
    OP_R_FETCH = 39,		// Copy from return stack

    /*
     * Parameter stack
     */
    OP_SP = 40,			// Stack pointer
    OP_DEPTH = 41,		// Number of elements
    OP_DROP = 42,		// Drop top of stack
    OP_NIP = 43,		// Drop next top of stack
    OP_EMPTY = 44,		// Empty stack
    OP_DUP = 45,		// Duplicate top of stack
    OP_QUESTION_DUP = 46,	// Duplicate top of stack if not zero
    OP_OVER = 47,		// Duplicate next top of stack
    OP_TUCK = 48,		// Duplicate top of stack and rotate
    OP_PICK = 49,		// Duplicate index stack element
    OP_SWAP = 50,		// Swap two top stack elements
    OP_ROT = 51,		// Rotate three top stack elements
    OP_MINUS_ROT = 52,		// Inverse rotate three top stack elements
    OP_ROLL = 53,		// Rotate given number of stack elements
    OP_TWO_SWAP = 54,		// Swap two double stack elements
    OP_TWO_DUP = 55,		// Duplicate double stack elements
    OP_TWO_OVER = 56,		// Duplicate double next top of stack
    OP_TWO_DROP = 57,		// Drop double top of stack

    /*
     * Constants
     */
    OP_MINUS_TWO = 58,		// Push constant(-2)
    OP_MINUS_ONE = 59,		// Push constant(-1)
    OP_ZERO = 60,		// Push constant(0)
    OP_ONE = 61,		// Push constant(1)
    OP_TWO = 62,		// Push constant(2)
    OP_CELL = 63,		// Stack width in bytes
    OP_CELLS = 64,		// Convert cells to bytes for allot

    /*
     * Bitwise logical operations
     */
    OP_BOOL = 65,		// Convert top of stack to boolean
    OP_NOT = 66,		// Convert top of stack to invert boolean
    OP_TRUE = 67,		// Push true(-1)
    OP_FALSE = 68,		// Push false(0)
    OP_INVERT = 69,		// Bitwise inverse top element
    OP_AND = 70,		// Bitwise AND top two elements
    OP_OR = 71,			// Bitwise OR top two elements
    OP_XOR = 72,		// Bitwise XOR top two elements

    /*
     * Arithmetic operations
     */
    OP_NEGATE = 73,		// Negate top of stack
    OP_ONE_PLUS = 74,		// Increment top of stack
    OP_ONE_MINUS = 75,		// Decrement top of stack
    OP_TWO_PLUS = 76,		// Increment by two
    OP_TWO_MINUS = 77,		// Decrement by two
    OP_TWO_STAR = 78,		// Multiply by two
    OP_TWO_SLASH = 79,		// Divide by two
    OP_PLUS = 80,		// Add top two elements
    OP_MINUS = 81,		// Substract top two elements
    OP_STAR = 82,		// Multiply top two elements
    OP_STAR_SLASH = 83,		// Multiply/Divide top three elements
    OP_SLASH = 84,		// Quotient for division of top two elements
    OP_MOD = 85,		// Remainder for division of top two elements
    OP_SLASH_MOD = 86,		// Quotient and remainder
    OP_LSHIFT = 87,		// Left shift
    OP_RSHIFT = 88,		// Right shift

    /*
     * Math operations
     */
    OP_WITHIN = 89,		// Within boundard
    OP_ABS = 90,		// Absolute value
    OP_MIN = 91,		// Minimum value
    OP_MAX = 92,		// Maximum value

    /*
     * Relational operations
     */
    OP_ZERO_NOT_EQUALS = 93,	// Not equal zero
    OP_ZERO_LESS = 94,		// Less than zero
    OP_ZERO_EQUALS = 95,	// Equal to zero
    OP_ZERO_GREATER = 96,	// Greater than zero
    OP_NOT_EQUALS = 97,		// Not equal
    OP_LESS = 98,		// Less than
    OP_EQUALS = 99,		// Equal
    OP_GREATER = 100,		// Greater than
    OP_U_LESS = 101,		// Unsigned less than

    /*
     * Dictionary functions
     */
    OP_LOOKUP = 102,		// Lookup word in dictionary
    OP_TO_BODY = 103,		// Access data area application variable
    OP_WORDS = 104,		// Print list of operations/functions

    /*
     * Basic I/O
     */
    OP_BASE = 105,		// Base for number conversion
    OP_HEX = 106,		// Set hexa-decimal number conversion base
    OP_DECIMAL = 107,		// Set decimal number conversion base
    OP_QUESTION_KEY = 108,	// Read character if available
    OP_KEY = 109,		// Wait for character and read
    OP_EMIT = 110,		// Print character
    OP_CR = 111,		// Print new-line
    OP_SPACE = 112,		// Print space
    OP_SPACES = 113,		// Print spaces
    OP_U_DOT = 114,		// Print top of stack as unsigned
    OP_DOT = 115,		// Print top of stack
    OP_DOT_S = 116,		// Print contents of parameter stack
    OP_DOT_QUOTE = 117,		// Print program memory string
    OP_TYPE = 118,		// Print string
    OP_DOT_NAME = 119,		// Print operation/function name
    OP_QUESTION = 120,		// Print value of variable

    /*
     * Arduino extensions
     */
    OP_MICROS = 121,		// Micro-seconds
    OP_MILLIS = 122,		// Milli-seconds
    OP_DELAY = 123,		// Delay milli-seconds (yield)
    OP_PINMODE = 124,		// Digital pin mode
    OP_DIGITALREAD = 125,	// Read digital pin
    OP_DIGITALWRITE = 126,	// Write digital pin
    OP_DIGITALTOGGLE = 127,	// Toggle digital pin
    OP_ANALOGREAD = 128,	// Read analog pin
    OP_ANALOGWRITE = 129,	// Write pwm pin

    /*
     * Max dictionary tokens
     * 0..127 	direct kernel words/switch, PROGMEM
     * 128..255	extended kernel words/prefix/function table, PROGMEM
     * 256..383 direct application words/function table, PROGMEM
     * 384..511 extended application words/prefix/function table, SRAM
     */
    CORE_MAX = 128,
    KERNEL_MAX = 256,
    FUNC_MAX = 384,
    TOKEN_MAX = 511
  };

  /** Cell data type. */
  typedef int16_t cell_t;
  typedef uint16_t ucell_t;

  /** Double cell data type. */
  typedef int32_t cell2_t;
  typedef uint32_t ucell2_t;

  /** Operation code/function index. */
  typedef int8_t code_t;
  typedef const PROGMEM code_t* code_P;

  struct task_t {
    Stream& m_ios;		// Input/Output stream
    cell_t m_base;		// Number conversion base
    bool m_trace;		// Trace mode
    code_P* m_rp;		// Return stack pointer
    code_P* m_rp0;		// Return stack bottom pointer
    cell_t* m_sp;		// Parameter stack pointer
    cell_t* m_sp0;		// Parameter stack bottom pointer

    /**
     * Construct task with given in-/output stream, stacks and
     * function pointer. Default number conversion base is 10,
     * and trace disabled.
     * @param[in] ios in-/output stream.
     * @param[in] sp0 bottom of parameter stack.
     * @param[in] rp0 bottom of return stack.
     * @param[in] fn function pointer.
     */
    task_t(Stream&ios, cell_t* sp0, code_P* rp0, code_P fn ) :
      m_ios(ios),
      m_base(10),
      m_trace(false),
      m_rp(rp0),
      m_rp0(rp0),
      m_sp(sp0 + 1),
      m_sp0(sp0)
    {
      *++m_rp = fn;
    }

    /**
     * Push value to parameter stack.
     * @param[in] value to push.
     */
    void push(cell_t value)
    {
      *++m_sp = value;
    }

    /**
     * Pop value from parameter stack.
     * @return value.
     */
    cell_t pop()
    {
      return (*m_sp--);
    }

    /**
     * Parameter stack depth.
     * @return depth.
     */
    int depth()
    {
      return (m_sp - m_sp0 - 1);
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
      *++m_rp = fn;
      return (*this);
    }
  };

  template<int PARAMETER_STACK_MAX, int RETURN_STACK_MAX>
  struct Task : task_t {
    cell_t m_params[PARAMETER_STACK_MAX];
    code_P m_returns[RETURN_STACK_MAX];

    /**
     * Construct task with given in-/output stream and function
     * pointer.
     * @param[in] ios in-/output stream.
     * @param[in] fn function pointer (default none).
     */
    Task(Stream& ios, code_P fn = 0) :
      task_t(ios, m_params, m_returns, fn)
    {}
  };

  /**
   * Wrapper for variable.
   */
  struct var_t {
    code_t op;			// OP_VAR
    cell_t* value;		// Pointer to value (RAM)
  };

  /**
   * Wrapper for constant.
   */
  struct const_t {
    code_t op;			// OP_CONST
    cell_t value;		// Value of constant (PROGMEM)
  };

  /**
   * Wrapper for extension functions.
   * @param[in] task.
   */
  typedef void (*fn_t)(task_t &task, void* env);
  struct func_t {
    code_t op;			// OP_FUNC
    fn_t fn;			// Pointer to function
    void* env;			// Pointer to environment (RAM)
  };

  /**
   * Construct forth virtual machine with given data area.
   * @param[in] dp0 initial data pointer.
   */
  FVM(uint8_t* dp0 = 0) :
    m_dp(dp0),
    m_dp0(dp0),
    m_latest(dp0)
  {
    *m_latest = 0;
  }

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
   * @param[in] dp data allocation pointer.
   */
  void dp(uint8_t* dp)
  {
    m_dp = dp;
  }

  /**
   * Get current latest word pointer.
   * @return pointer.
   */
  uint8_t* latest()
  {
    return (m_latest);
  }

  /**
   * Link latest dictionary entry.
   */
  void link()
  {
    *m_latest = m_dp - m_latest;
    m_latest = m_dp;
  }

  /**
   * Forget dictionary entry.
   */
  void forget(uint8_t* dp = 0)
  {
    if (dp == 0) dp = m_dp0;
    m_latest = dp;
    *m_latest = 0;
    m_dp = m_dp0;
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
   * Lookup given string in dictionary. Return operation code
   * (0..KERNEL_MAX-1) or function index (KERNEL_MAX..SKETCH_MAX),
   * otherwise negative error code(-1).
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
  int execute(int op, task_t& task);

  /**
   * Execute given function code pointer with task. Returns
   * on yield(1), halt(0) or illegal instruction (-1).
   * @param[in] fn function code pointer.
   * @param[in] task to resume.
   * @return error code.
   */
  int execute(code_P fn, task_t& task)
  {
    return (resume(task.call(fn)));
  }

  /**
   * Lookup given name in dictionary and execute with given task.
   * Convert to number if possible, and push on task stack.
   * @param[in] name of function to execute.
   * @param[in] task to resume.
   * @return error code.
   */
  int execute(const char* name, task_t& task)
  {
    return (execute(lookup(name), task));
  }

  /**
   * Interpret; scan, lookup and execute.
   * @param[in] task to run.
   * @return error code.
   */
  int interpret(task_t& task);

  // Function code and dictionary provided by sketch
  static const code_P fntab[] PROGMEM;
  static const str_P fnstr[] PROGMEM;

 protected:
  // Kernel dictionary
  static const str_P opstr[] PROGMEM;

  // Data allocation pointer
  uint8_t* m_dp;
  uint8_t* m_dp0;

  // Sketch dictionary
  uint8_t* m_latest;
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
 * Compile tail call to given function in function table.
 * @param[in] fn function index in table.
 */
#define FVM_TAIL(fn)							\
  FVM::OP_TAIL,								\
  FVM_CALL(fn)

/**
 * Create a named reference to a created object.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] does object handler (function).
 * @param[in] data storage.
 */
#define FVM_CREATE(id,var,does,data)					\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = #data;				\
  const FVM::var_t var ## _VAR PROGMEM = {				\
    FVM_CALL(does),							\
    (FVM::cell_t*) &data						\
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
    (FVM::cell_t*) &data						\
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
 * @param[in] env environment.
*/
#define FVM_FUNCTION(id,var,fn,env)					\
  const int var = id;							\
  const char var ## _PSTR[] PROGMEM = #fn;				\
  const FVM::func_t var ## _FUNC PROGMEM = {				\
    FVM_OP(FUNC),							\
    fn,									\
    (FVM::cell_t*) &env							\
  }

/**
 * Create a name table symbol.
 * @param[in] id identity index.
 * @param[in] var variable name.
 * @param[in] name dictionary string.
 */
#define FVM_SYMBOL(id,var,name)						\
  const int var = id + FVM::KERNEL_MAX;					\
  const char var ## _PSTR[] PROGMEM = name

#endif
