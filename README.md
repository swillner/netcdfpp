# netcdfpp

netcdfpp is a small header-only C++ wrapper around the NetCDF-C API. It is not
trying to hide NetCDF behind a new model. The idea is simply to make the C API a
bit nicer to use from C++ code.

You still work with files, groups, dimensions, variables, attributes, and
NetCDF types. netcdfpp adds the things I usually want around that: files close
themselves, reads and writes are typed, lookup failures can be handled
explicitly, and NetCDF errors become exceptions that include the path of the
object that failed.

The common path is meant to stay short. Define a dimension, use it to create a
variable, write a vector. Or look up a variable, check its dimensions, and read
the values back as the C++ type you asked for. The surrounding NetCDF details
are still there when you need them, but the everyday read/write code is less
noisy.

## Why?

I often want NetCDF-C, but not another C++ library on top. `netcdf-cxx4` is the
usual choice, and it is useful, but it is also one more compiled dependency to
package and link.

netcdfpp is for the smaller case: one header, linked directly against
NetCDF-C. Define dimensions, attach variables, write arrays and attributes, and
read them back without passing raw ids and buffer pointers through the whole
program.

## Quick example

```cpp
#include "netcdfpp.h"

int main() {
    netCDF::File file("example.nc", 'w');

    auto x = file.add_dimension_variable<double>("x", 3);
    x.set<double>({0.0, 1.0, 2.0});

    auto values = file.add_variable<double>("values", {"x"});

    values.set<double>({1.0, 2.0, 3.0});
    file.add_attribute("title").set<std::string>("example");
}
```

And reading the same file:

```cpp
netCDF::File file("example.nc", 'r');

auto values = file.variable("values").require();
std::vector<double> data = values.get<double>();

auto title = file.attribute("title").require().get_string();
```

For gridded data this stays fairly direct:

```cpp
auto lat = file.add_dimension_variable<double>("lat", 3);
auto lon = file.add_dimension_variable<double>("lon", 4);

lat.set<double>({50.0, 51.0, 52.0});
lon.set<double>({7.0, 8.0, 9.0, 10.0});

auto temperature = file.add_variable<float>("temperature", {"lat", "lon"});
temperature.set<float>({
    12.0f, 12.5f, 13.0f, 13.5f,
    11.0f, 11.5f, 12.0f, 12.5f,
    10.0f, 10.5f, 11.0f, 11.5f,
});
```

Lookups return `Maybe<T>`. That makes it possible to either check whether
something exists, or call `require()` when its absence should be an error:

```cpp
if (auto maybe_var = file.variable("temperature")) {
    auto temperature = maybe_var.require();
}
```

## CMake usage

The repository includes a small CMake helper:

```cmake
include(path/to/netcdfpp.cmake)
include_netcdfpp(my_target)
```

This adds the header path and links the target to NetCDF-C.

## File modes

`netCDF::File` uses a deliberately small mode interface:

- `'r'`: open an existing file read-only;
- `'a'`: open an existing file read-write;
- `'w'`: create a NetCDF4 file, replacing an existing file.

Most operations throw `netCDF::Exception` when the NetCDF-C API returns an
error. The exception message includes the affected object path, and
`return_code()` exposes the original NetCDF return code.

## What is covered?

The wrapper covers the parts of NetCDF that I tend to need in C++ code:
ordinary variables and attributes, groups and dimensions, NetCDF strings,
compound types, enums, opaque types, variable-length values, and helpers for
copying structure and data between files.

The public interface is C++14 and header-only. The only library it links to is
NetCDF-C.

## Tests

The tests use the vendored `doctest` framework in `lib/doctest`:

```sh
cmake -S . -B build
cmake --build build --target test
```

## Documentation

If Doxygen is installed, the API documentation can be generated with:

```sh
cmake -S . -B build
cmake --build build --target docs
```

The generated documentation is written to `build/docs/html`.
