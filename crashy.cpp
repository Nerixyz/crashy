#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <Windows.h>
#include <Psapi.h>
// clang-format on

using namespace std::string_view_literals;
namespace views = std::views;
namespace ranges = std::ranges;
namespace fs = std::filesystem;

// helper for std::visit
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

constexpr const size_t N_PROCESSES = 1024;

// TODO: wait for GitHub CI to have MSVC 14.37 or later (14.35 currently)
void println(const auto &data) { std::cout << data << std::endl; }

[[noreturn]] void fail() { std::exit(1); }
[[noreturn]] void fail(const auto &msg) {
  println(std::format("Unable to continue: {}", msg));
  std::exit(1);
}

auto isNumeric(std::string_view string) -> bool {
  return ranges::all_of(string,
                        [](auto dig) { return dig >= '0' && dig <= '9'; });
}

struct Args {
  std::string_view payloadPath;
  std::variant<DWORD, std::string_view> executable;

  static auto parse(std::span<const char *> args) -> Args;
};

class ManagedHandle {
public:
  ManagedHandle(HANDLE handle) : handle_(handle) {}
  ~ManagedHandle() {
    if (this->handle_ != nullptr) {
      CloseHandle(this->handle_);
    }
  }
  auto operator()() -> HANDLE { return this->handle_; }

private:
  HANDLE handle_;
};

auto findPid(std::string_view name) -> std::optional<DWORD> {
  std::array<DWORD, N_PROCESSES> buffer;
  DWORD needed;

  if (!EnumProcesses(buffer.data(), buffer.size() * sizeof(DWORD), &needed)) {
    return std::nullopt;
  }

  std::vector<char> nameBuf;
  nameBuf.resize(name.length() + 2);

  for (auto pid : buffer | views::take(needed / sizeof(DWORD))) {
    ManagedHandle handle =
        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (handle() == nullptr) {
      continue;
    }

    HMODULE module;
    DWORD neededModules;
    if (!EnumProcessModulesEx(handle(), &module, sizeof(HMODULE),
                              &neededModules, LIST_MODULES_ALL)) {
      continue;
    }
    GetModuleBaseNameA(handle(), module, nameBuf.data(), nameBuf.size());
    if (name == nameBuf.data()) {
      return pid;
    }
  }

  return std::nullopt;
}

void inject(std::string_view path, DWORD pid) {
  ManagedHandle process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_WRITE |
                                          PROCESS_VM_OPERATION,
                                      FALSE, pid);
  if (process() == nullptr) {
    fail("Failed to open process");
  }

  // Copy absolute path into process memory
  std::string canonical;
  try {
    canonical = fs::canonical(path).string();
  } catch (const fs::filesystem_error &ex) {
    println(std::format("Failed to canonicalize payload path (does the payload "
                        "exist?): {} (code: {})",
                        ex.what(), ex.code().value()));
    fail();
  }

  void *pathBuf = VirtualAllocEx(process(), nullptr, canonical.size() + 1,
                                 MEM_COMMIT, PAGE_READWRITE);
  if (pathBuf == nullptr) {
    fail("Failed to allocate memory for the path inside the process");
  }

  if (WriteProcessMemory(process(), pathBuf, canonical.data(),
                         canonical.size() + 1, nullptr) != TRUE) {
    fail("Failed to write path into process");
  }

  // Call LoadLibraryA(canonical) inside the process
  auto *module = GetModuleHandleW(L"kernel32.dll");
  if (module == nullptr) {
    fail("Failed to load kernel32.dll");
  }
  auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
      GetProcAddress(module, "LoadLibraryA"));
  if (loadLibrary == nullptr) {
    fail("Failed to get address of LoadLibraryA");
  }

  ManagedHandle thread = CreateRemoteThread(process(), nullptr, 0, loadLibrary,
                                            pathBuf, 0, nullptr);
  if (thread() == nullptr) {
    fail("Failed to call CreateRemoteThread");
  }
  WaitForSingleObject(thread(), INFINITE);
}

auto main(int argc, const char *argv[]) -> int {
  auto args = Args::parse({argv, static_cast<size_t>(argc)});

  auto pid = std::visit(
      overloaded{
          [](DWORD pid) { return pid; },
          [](std::string_view name) {
            auto pid = findPid(name);
            if (!pid) {
              fail("Failed to find process by name, consider specifying a PID");
            }
            return *pid;
          }},
      args.executable);
  inject(args.payloadPath, pid);
  return 0;
}

auto Args::parse(std::span<const char *> args) -> Args {
  std::optional<std::string_view> payloadPath;
  std::optional<std::variant<DWORD, std::string_view>> executable;

  bool seenFlag = false;
  for (const auto &arg : args | views::drop(1) | views::transform([](auto arg) {
                           return std::string_view(arg);
                         })) {
    if (seenFlag) {
      if (payloadPath) {
        fail("Duplicate -p");
      }
      payloadPath = arg;
      seenFlag = false;
      continue;
    }

    if (arg == "-p"sv) {
      seenFlag = true;
      continue;
    }

    if (arg == "-h"sv) {
      println("crashy.exe <target (PID or executable)> [-p <payload-path>]");
      std::exit(0);
    }

    if (executable) {
      fail("Duplicate target");
    }

    if (isNumeric(arg)) {
      DWORD pid = 0;
      auto [ptr, ec] =
          std::from_chars(arg.data(), arg.data() + arg.size(), pid);
      executable = pid;
    } else {
      executable = arg;
    }
  }

  if (seenFlag) {
    fail("Trailing -p");
  }
  if (!executable) {
    fail("No executable given");
  }

  return Args{
      .payloadPath = payloadPath.value_or("crashy-payload.dll"sv),
      .executable = *executable,
  };
}
