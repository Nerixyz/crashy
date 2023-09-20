#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <print>
#include <thread>

auto WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason,
                    LPVOID /*lpvReserved*/) -> BOOL {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    std::thread([] { std::print("{}", (int)(*((volatile int *)0))); }).detach();
  }

  return TRUE;
}
