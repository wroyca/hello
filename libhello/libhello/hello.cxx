#include <libhello/hello.hxx>
#include <libhello/win32-utility.hxx>

#include <cassert>

BOOL WINAPI
DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;

  // For `DLL_PROCESS_DETACH`, the `lpReserved` parameter is used to determine
  // the context:
  //
  //   - `lpReserved == NULL` when `FreeLibrary()` is called.
  //   - `lpReserved != NULL` when the process is being terminated.
  //
  // When `FreeLibrary()` is called, worker threads remain alive. That is,
  // runtime's state is consistent, and executing proper shutdown is
  // acceptable.
  //
  // When process is terminated, worker threads have either exited or been
  // forcefully terminated by the OS, leaving only the shutdown thread. This
  // situation leave runtime in an inconsistent state.
  //
  // Hence, proper cleanup should only be attempted when `FreeLibrary()` is
  // called. Otherwise, rely on the OS to reclaim resources.
  //
  case DLL_PROCESS_DETACH:
    {
      if (lpvReserved != nullptr)
      {
        // Do not do cleanup if process termination scenario.
        //
        break;
      }

      // Hello is typically not loaded using `LoadLibrary()` by default, so it should generally not
      // reach a situation where `FreeLibrary()` is called on its handle.
      //
      [[fallthrough]];
    }

  default:
    assert (FALSE);
  }

  // Successful DLL_PROCESS_ATTACH.
  //
  return TRUE;
}
