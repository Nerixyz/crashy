# Crashy

This is a simple app that can be used to crash other processes on Windows.
It's intended for debugging and generating crashdumps/minidumps (i.e. to be used on apps using breakpad or crashpad or similar).

Download the app and its payload from the [releases tab](https://github.com/Nerixyz/crashy/releases).

## Usage

```ps
crashy.exe <target (PID or executable)> [-p <payload-path>]
```

`<payload-path>` will default to `crashy-payload.dll`.

## Building

A recent C++ compiler and CMake are required. This example uses `ninja` as the generator.

```ps
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd build
ninja all
```
