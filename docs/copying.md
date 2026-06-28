# Copying {#copying}

Groups, variables, dimensions, attributes, and user-defined types can be copied between files and groups.

```cpp
netCDF::File in("input.nc", 'r');
netCDF::File out("output.nc", 'w');

out.copy_from(in, true);
```

The second argument controls whether variable values are copied. Metadata is copied first. When copying user-defined types between groups, netcdfpp resolves the matching type in the destination group before copying variables or attributes that use it.

Copying variable values checks type size, dimension count, and dimension sizes before reading and writing the raw data. A mismatch throws `netCDF::Exception`.
