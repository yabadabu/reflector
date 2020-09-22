#pragma once

#include <cstdarg>
#include <cstdio>
#include "strs.h"

void dbg(const char* fmt, ...);
int fatal(const char* fmt, ...);

typedef void(*TSysOutputHandler)(const char* error);
TSysOutputHandler dbg_handler;
TSysOutputHandler fatal_handler;

int fatal(const char* fmt, ...) {
  if (!fatal_handler)
    return 0;

  TStr<4096> buf;
  va_list argp;
  va_start(argp, fmt);
  buf.formatVaList(fmt, argp);
  va_end(argp);
  
  (*fatal_handler)(buf);
  return 0;
}
