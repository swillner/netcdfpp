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

For compound types, the NetCDF field layout must match the C++ object layout you write and read:

```cpp
struct Point {
    double x;
    double y;
};

auto n = file.add_dimension("n", 2);
auto point = file.add_type_compound<Point>("Point");
point.add_compound_field<double>("x", offsetof(Point, x));
point.add_compound_field<double>("y", offsetof(Point, y));

auto points = file.add_variable("points", point, {n});
points.set<Point>({{1.0, 2.0}, {3.0, 4.0}});
```

Reading can check both the variable shape and the compound type before loading the data:

```cpp
auto points = file.variable("points").require()
    .require_dimensions({"n"})
    .require_compound<Point>(2);

std::vector<Point> values = points.get<Point>();
```

The same `UserType` object is used when storing user-defined values in attributes:

```cpp
file.add_attribute("origin").set<Point>(Point{0.0, 0.0}, point);
```

Enums use the C++ enum's underlying type unless an explicit NetCDF base type is supplied:

```cpp
enum class Quality : int {
    good = 1,
    suspect = 2,
};

auto quality = file.add_type_enum<Quality>("Quality");
quality.add_enum_member("good", Quality::good);
quality.add_enum_member("suspect", Quality::suspect);

auto flags = file.add_variable("quality", quality, {"n"});
flags.set<Quality>({Quality::good, Quality::suspect});
```

Opaque types are useful for fixed-size binary records:

```cpp
struct Packet {
    unsigned char bytes[16];
};

auto packet = file.add_type_opaque("Packet", sizeof(Packet));
auto packets = file.add_variable("packets", packet, {"n"});
```

Variable-length types are defined from a base type. When writing values, pass the `nc_vlen_t` shape expected by NetCDF-C:

```cpp
auto samples = file.add_type_vlen<int>("Samples");
auto traces = file.add_variable("traces", samples, {"n"});

std::vector<int> trace = {1, 2, 3};
traces.set<nc_vlen_t, 1>({trace.size(), trace.data()}, {0});
```

For variable-length values returned by NetCDF, use `VLenElement<T>`. It owns the memory allocated by the NetCDF-C library and releases it when the element is destroyed.

```cpp
auto trace = traces.get<netCDF::VLenElement<int>, 1>({0});
```
