#pragma once

#include <libhello/win32-utility.hxx>

namespace hello
{
  extern "C"
  {
    // Generic entry point into our dynamic-link library.
    //
    BOOL WINAPI
    DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
  }
}
