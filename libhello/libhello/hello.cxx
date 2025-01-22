#include <libhello/hello.hxx>

#include <array>
#include <cassert>
#include <cstdint>

namespace hello
{
  namespace
  {
    void
    main ()
    {
      MessageBox (nullptr, "Hello", "...", MB_OK);
    }
  }

  constexpr uintptr_t security_init_cookie = 0x000000; // FIXME: Update this address manually
  constexpr uintptr_t tmainCRTStartup = 0x000000;      // FIXME: Update this address manually

  [[noreturn]] __attribute__ ((naked)) void
  start ()
  {
    __asm__ __volatile__
    (
      // Save registers manually
      //
      "push %%eax\n\t"
      "push %%ecx\n\t"
      "push %%edx\n\t"
      "push %%ebx\n\t"
      "push %%esp\n\t"
      "push %%ebp\n\t"
      "push %%esi\n\t"
      "push %%edi\n\t"

      // Call security_init_cookie
      //
      "call *%0\n\t"

      // Call main
      //
      "call *%1\n\t"

      // Restore registers manually
      //
      "pop %%edi\n\t"
      "pop %%esi\n\t"
      "pop %%ebp\n\t"
      "pop %%esp\n\t"
      "pop %%ebx\n\t"
      "pop %%edx\n\t"
      "pop %%ecx\n\t"
      "pop %%eax\n\t"

      // Jump to tmainCRTStartup
      //
      "jmp *%2\n\t"
      :
      : "r"(security_init_cookie), "r"(main), "r"(tmainCRTStartup)
      : "memory");
  }

  extern "C"
  {
    // DllMain runs inside the loader lock.
    //
    // This is one of the few times the OS lets us run code while one of its
    // internal locks is held. This means that we *must* be extra careful not
    // to violate a lock hierarchy.
    //
    BOOL WINAPI
    DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
    {
      switch (fdwReason)
      {
      case DLL_PROCESS_ATTACH:
        {
          const uintptr_t target = 0x000000;
          const uintptr_t source = (uintptr_t)&start;
          const uintptr_t offset = source - target - 5;

          unsigned long old_protect (0);

          VirtualProtect (reinterpret_cast<LPVOID> (target),
                          0x05,
                          PAGE_EXECUTE_READWRITE,
                          &old_protect);
          {
            std::array<uint8_t, 5> jump_instruction = {
              {0xE9,
               static_cast<uint8_t> (offset),
               static_cast<uint8_t> (offset >> 8),
               static_cast<uint8_t> (offset >> 16),
               static_cast<uint8_t> (offset >> 24)}};

            memcpy (reinterpret_cast<void*> (target),
                    jump_instruction.data (),
                    jump_instruction.size ());

            VirtualProtect (reinterpret_cast<LPVOID> (target),
                            0x05,
                            old_protect,
                            &old_protect);

            FlushInstructionCache (GetCurrentProcess (),
                                   reinterpret_cast<LPCVOID> (target),
                                   jump_instruction.size ());
          }
          break;
        }

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;

      // For `DLL_PROCESS_DETACH`, the `lpReserved` parameter is used to
      // determine the context:
      //
      //   - `lpReserved == NULL` when `FreeLibrary()` is called.
      //   - `lpReserved != NULL` when the process is being terminated.
      //
      // When `FreeLibrary()` is called, worker threads remain alive. That is,
      // runtime's state is consistent, and executing proper shutdown is
      // acceptable.
      //
      // When process is terminated, worker threads have either exited or been
      // forcefully terminated by the OS, leaving only the shutdown thread.
      // This situation leave runtime in an inconsistent state.
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

          // Hello is typically not loaded using `LoadLibrary()` by default, so
          // it should generally not reach a situation where `FreeLibrary()` is
          // called on its handle.
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
  }
}

// When working with static Thread-Local Storage in MinGW, we may encounter a
// protection fault during relocation. This occurs because MinGW runtime
// includes the tlssup.o object file with the CRT initialization routines,
// which initializes TLS.
//
// More specifically, this happens due to the __dyn_tls_init_callback hook,
// declared in crtdll.c and defined in tlssup.c. Since this hook is non-null by
// default, the linker adds a .tls section to the binary, which assumes a fixed
// base address.
//
// For what it’s worth, other CRTs resolve this by leaving the TLS hook
// uninitialized unless __declspec(thread) variables are used. MinGW, however,
// doesn't handle it this way, so we need to take steps to avoid including
// tlssup.o.
//
#ifdef __MINGW32__
  const void* __dyn_tls_init_callback = nullptr;
#endif
