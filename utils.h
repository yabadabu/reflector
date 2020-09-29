#pragma once

#include <cstdarg>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

void dbg(const char* fmt, ...);
int fatal(const char* fmt, ...);

void myDbgHandler(const char* txt) {
  printf("%s", txt);
  #ifdef PLATFORM_WINDOWS
  ::OutputDebugString(txt);
  #endif
}

void myFatalHandler(const char* txt) {
  myDbgHandler(txt);
  #ifdef PLATFORM_WINDOWS
  if (MessageBox(nullptr, txt, "Error", MB_RETRYCANCEL) == IDCANCEL) {
    __debugbreak();
  }
  #else
    assert(false);
    exit(-1);
  #endif
}

typedef void(*TSysOutputHandler)(const char* error);
TSysOutputHandler dbg_handler = &myDbgHandler;
TSysOutputHandler fatal_handler = &myFatalHandler;

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
