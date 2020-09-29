#pragma once

#include <cstdarg>
#include <cstdio>

void dbg(const char* fmt, ...);
int fatal(const char* fmt, ...);

typedef void(*TSysOutputHandler)(const char* error);
TSysOutputHandler dbg_handler;
TSysOutputHandler fatal_handler;

int fatal(const char* fmt, ...) {
  if (!fatal_handler)
    return 0;

  char buf[4096];
  va_list argp;
  va_start(argp, fmt);
  vsnprintf( buf, sizeof(buf), fmt, argp );
  va_end(argp);
  
  (*fatal_handler)(buf);
  return 0;
}

void dbg(const char* fmt, ...) {
  if (!dbg_handler)
    return;

  char buf[4096];
  va_list argp;
  va_start(argp, fmt);
  vsnprintf( buf, sizeof(buf), fmt, argp );
  va_end(argp);

  (*dbg_handler)(buf);
}
