# netcdfpp

netcdfpp is a small header-only C++ wrapper around the NetCDF-C API. It keeps the NetCDF data model visible while adding RAII file handling, typed reads and writes, convenient lookup helpers, and exceptions that include the affected object path.

The public API lives in `include/netcdfpp.h` under the `netCDF` namespace. Operations that call the NetCDF-C library throw `netCDF::Exception` when the C API reports an error.

## Getting started

Include the project with CMake:

```cmake
include(netcdfpp.cmake)
include_netcdfpp(my_target)
```

Then include the header:

```cpp
#include "netcdfpp.h"
```

Common tasks:

- @subpage quickstart
- @subpage types
- @subpage copying

## Building the documentation

Configure the project with CMake and build the `docs` target:

```sh
cmake -S . -B build
cmake --build build --target docs
```

The HTML output is written to `build/docs/html`.
