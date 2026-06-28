# Quickstart {#quickstart}

## Create a file

```cpp
#include "netcdfpp.h"

int main() {
    netCDF::File file("example.nc", 'w');
    auto x = file.add_dimension("x", 3);
    auto values = file.add_variable<double>("values", {x});

    values.set<double>({1.0, 2.0, 3.0});
    file.add_attribute("title").set<std::string>("example");
}
```

Mode `'w'` creates a NetCDF4 file and clobbers an existing file. Mode `'r'` opens read-only, and mode `'a'` opens read-write.

## Read data

```cpp
netCDF::File file("example.nc", 'r');
auto values = file.variable("values").require();
std::vector<double> data = values.get<double>();
```

Lookups such as `Group::variable`, `Group::dimension`, and `Group::attribute` return `Maybe<T>`. Use it in a boolean context for optional lookups or call `require()` to get the object and throw a `netCDF::Exception` if it is missing.

```cpp
if (auto maybe_title = file.attribute("title")) {
    std::string title = maybe_title.require().get_string();
}
```

## Indexed access

Variables support full reads/writes, single-element access, hyperslabs, strided access, and mapped access. The `std::array` overloads are the most convenient for fixed-rank variables:

```cpp
auto value = values.get<double, 1>({2});
values.set<double, 1>(4.0, {2});
```
