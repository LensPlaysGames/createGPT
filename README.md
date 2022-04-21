# createGPT
Easily create disk images with valid GUID partition tables, cross-platform.

---

### Using <a name="using"></a>
Call `createGPT` with no arguments for general help on usage and flags.

Add a partition using the following flag. Specify it multiple times to add multiple partitions.
```sh
--part <path> [--type <GUID|preset>]
```
`<path>` is a filepath to the contents of the partition.

`--type` can be passed either a GUID, or a type preset.
Presets:
- `null` -- `00000000-0000-0000-0000-000000000000`
- `system` -- `c12a7328-f81f-11d2-ba4b-00a0c93e3b00`

When passing GUIDs over the command line, use the following format:
```
00000000-0000-0000-0000-000000000000
```
Put the values in mixed-endian, [canonical fashion](https://en.wikipedia.org/wiki/Universally_unique_identifier#Format).

---

### Building <a name="building"></a>

- [CMake](https://www.cmake.org/download/)
- Any C compiler (MSVC, GCC, Clang)

Generate a build system:
```sh
cmake -S . -B bld
```

Invoke the generated build system to generate an executable:
```sh
cmake --build bld
```

---
