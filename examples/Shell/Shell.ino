/**
 * @file FVM/Shell.ino
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
 * Basic interactive shell with the Forth Virtual Machine (FVM).
 */

#include "FVM.h"

const FVM::code_P FVM::fntab[] PROGMEM = {};
const str_P FVM::fnstr[] PROGMEM = { 0 };

uint8_t data[128];
FVM fvm(data);
FVM::task_t task(Serial);

void setup()
{
  Serial.begin(57600);
  while (!Serial);
  Serial.println(F("FVM/Shell: started [Newline]"));
}

void loop()
{
  // Scan buffer for a single word or number
  char buffer[32];
  char pad[32];
  char* bp = buffer;
  char c;

  // Skip white space
  do {
    while (!Serial.available());
    c = Serial.read();
  } while (c <= ' ');

  // Scan until white space
  do {
    *bp++ = c;
    while (!Serial.available());
    c = Serial.read();
   } while (c > ' ');
  *bp = 0;

  // Check for operation/function name
  if (buffer[0] == '\\') {
    strcpy(pad, buffer+1);
    task.push((int) pad);
  }
  // Lookup and execute
  else
    fvm.execute(buffer, task);

  // Print stack contents after each command line
  if (c == '\n' && !task.trace())
    fvm.execute(FVM::OP_DOT_S, task);
}
