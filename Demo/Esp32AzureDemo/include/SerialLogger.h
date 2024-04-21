// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SERIALLOGGER_H
#define SERIALLOGGER_H

#include <Arduino.h>

#ifndef SERIAL_LOGGER_BAUD_RATE
#define SERIAL_LOGGER_BAUD_RATE 115200
#endif

class SerialLogger
{
public:
  SerialLogger();
  void Info     (String message);
  void Error    (String message);
  void Print    (String message);
  void PrintLn  (String message);
  void Info     (char * pMessage);
  void Error    (char * pMessage);
  void Print    (char * pMessage);
  void PrintLn  (char * pMessage);
  void Info     (const char * pMessage);
  void Error    (const char * pMessage);
  void Print    (const char * pMessage);
  void PrintLn  (const char * pMessage);
};

extern SerialLogger Logger;

#endif // SERIALLOGGER_H
