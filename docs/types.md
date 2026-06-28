# Types {#types}

netcdfpp maps C++ types to NetCDF atomic types with `netCDF::Type<T>`. Common numeric C++ types are supported directly, including signed and unsigned integer widths, `float`, and `double`.

## Strings

Use `std::string` for NetCDF string variables and attributes:

```cpp
auto names = file.add_variable<std::string>("names", {"x"});
names.set<std::string>({"a", "b", "c"});
```

Text attributes can be read with `Attribute::get_string()`. NetCDF string attributes can be read with `Attribute::get<std::string>()`.

## User-defined types

`UserType` supports NetCDF compound, enum, opaque, and variable-length types. Define the type on a group or file first, then use it when adding variables or attributes.

```cpp
struct Point {
    double x;
    double y;
};

auto point = file.add_type_compound<Point>("Point");
point.add_compound_field<double>("x", offsetof(Point, x));
point.add_compound_field<double>("y", offsetof(Point, y));

auto points = file.add_variable("points", point, {"n"});
```

For variable-length values returned by NetCDF, use `VLenElement<T>`. It owns the memory allocated by the NetCDF-C library and releases it when the element is destroyed.
