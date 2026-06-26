## Supported Environments
ParTest is intended to compile cleanly on any environment that fully supports any specified C++ dialect from C++11 onward. In practice, compiler and standard library support varies for newer features; a toolchain may declare standard conformance before the full feature set is implemented.

For example, <format> was missing from GCC's standard library prior to version 13. As a result, ParTest does not support C++20 on older versions of GCC.

The following environments are actively tested:

### Windows 10/11
MSVC 19.37+: C++14/17/20

### Ubuntu 22.04/24.04
G++11/12: C++11/14/17
G++13/14: C++11/14/17/20

Clang++11/12: C++11/14/17
Clang++18: C++11/14/17/20

## Unique pointers
ParTest uses `partest::make_unique<T>` whenever a unique pointer is created. For C++14 and later, this is an alias for `std::make_unique<T>`. For C++11 compatibility, an equivalent implementation is provided locally.

If your project must build against C++11, use `partest::make_unique<T>`. Otherwise, the standard library version is equivalent and either may be used.