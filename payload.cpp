#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>

auto WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason,
                    LPVOID /*lpvReserved*/) -> BOOL {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    std::thread([] { (*((volatile int *)nullptr)) = 0; }).detach();
  }

  return TRUE;
}
