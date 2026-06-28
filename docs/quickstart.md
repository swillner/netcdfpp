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

## Variables with dimensions

Coordinate variables are the usual NetCDF pattern where a dimension and a variable share a name. `add_dimension_variable<T>` defines both at once:

```cpp
netCDF::File file("grid.nc", 'w');

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

Reading it back keeps the shape checks near the code that depends on them:

```cpp
netCDF::File file("grid.nc", 'r');

auto temperature = file.variable("temperature").require()
    .require_dimensions({"lat", "lon"});

std::vector<float> all_values = temperature.get<float>();
float first_cell = temperature.get<float, 2>({0, 0});
std::vector<float> first_row = temperature.get<float, 2>({0, 0}, {1, 4});
```
