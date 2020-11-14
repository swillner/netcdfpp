#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
// make sure doctest comes before including tested classes

#include "netcdfpp.h"

struct TypeCompound {
    char c;
    int i[3][2];
    double d;
    bool operator==(const TypeCompound& rhs) const {
        return c == rhs.c && i[0][0] == rhs.i[0][0] && i[0][1] == rhs.i[0][1] && i[1][0] == rhs.i[1][0] && i[1][1] == rhs.i[1][1] && i[2][0] == rhs.i[2][0]
               && i[2][1] == rhs.i[2][1] && d == rhs.d;
    }
};

struct TypeOpaque {
    char c;
    bool operator==(const TypeOpaque& rhs) const { return c == rhs.c; }
};

enum class Enum : int {
    E1 = 1,
    E2 = 2,
    E3 = 3,
};

namespace netCDF {
namespace testing {
struct TestUserType {
    static netCDF::UserType create() {
        netCDF::UserType res(std::make_shared<netCDF::detail::Path>(netCDF::detail::Path{"fake_user_type", 0, true, nullptr}));
        res.fields_read = true;
        res.typeclass_m = NC_INT;
        return res;
    }
};
}  // namespace testing
}  // namespace netCDF

template<typename T, typename U>
void REQUIRE_OBJ(const T& o1, const U& o2) {
    REQUIRE(o1.id() == o2.id());
    REQUIRE(o1.name() == o2.name());
}

template<typename T>
void REQUIRE_VEC(const std::vector<T>& actual, const std::vector<std::string>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        REQUIRE(actual[i].name() == expected[i]);
    }
}

TEST_CASE("for_type helper function") {
    // require c++14
    netCDF::for_type<void>(NC_STRING, [](auto type) -> void { REQUIRE(std::is_same<decltype(type), char*>::value);});
    netCDF::for_type<void>(NC_CHAR, [](auto type) { REQUIRE(std::is_same<decltype(type), char>::value); });
    netCDF::for_type<void>(NC_DOUBLE, [](auto type) { REQUIRE(std::is_same<decltype(type), double>::value); });
    netCDF::for_type<void>(NC_FLOAT, [](auto type) { REQUIRE(std::is_same<decltype(type), float>::value); });
    netCDF::for_type<void>(NC_SHORT, [](auto type) { REQUIRE(std::is_same<decltype(type), std::int16_t>::value); });
    netCDF::for_type<void>(NC_INT, [](auto type) { REQUIRE(std::is_same<decltype(type), std::int32_t>::value); });
    netCDF::for_type<void>(NC_INT64, [](auto type) { REQUIRE(std::is_same<decltype(type), std::int64_t>::value); });
    netCDF::for_type<void>(NC_BYTE, [](auto type) { REQUIRE(std::is_same<decltype(type), std::int8_t>::value); });
    netCDF::for_type<void>(NC_USHORT, [](auto type) { REQUIRE(std::is_same<decltype(type), std::uint16_t>::value); });
    netCDF::for_type<void>(NC_UINT, [](auto type) { REQUIRE(std::is_same<decltype(type), std::uint32_t>::value); });
    netCDF::for_type<void>(NC_UINT64, [](auto type) { REQUIRE(std::is_same<decltype(type), std::uint64_t>::value); });
    netCDF::for_type<void>(NC_UBYTE, [](auto type) { REQUIRE(std::is_same<decltype(type), std::uint8_t>::value); });
    REQUIRE_THROWS_WITH_AS(netCDF::for_type<void>(-1, [](auto /* type */) {});, "Unsupported type", std::runtime_error);
}

TEST_CASE("invalid open") {
    netCDF::File file;
    try {
        file.attribute("file_not_open");
    } catch (const netCDF::Exception& e) {
        REQUIRE(e.return_code() == NC_EBADID);
    }
    REQUIRE_THROWS_WITH_AS(file.attribute("file_not_open"), "NetCDF: Not a valid ID: ", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.dimension("file_not_open"), "NetCDF: Not a valid ID: ", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.group("file_not_open"), "NetCDF: Not a valid ID: ", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.user_type("file_not_open"), "NetCDF: Not a valid ID: ", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.variable("file_not_open"), "NetCDF: Not a valid ID: ", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.open("test_non_existent.nc", 'r'), "No such file or directory: test_non_existent.nc", netCDF::Exception);
    REQUIRE_THROWS_WITH_AS(file.open("", 'X'), "Unknown file mode", std::runtime_error);
}

TEST_CASE("writing") {
    netCDF::File file("test.nc", 'w');

    // dimensions

    file.add_dimension("dim_unlimited");
    auto dim_with_size = file.add_dimension("dim_with_size", 10);
    file.add_dimension("dim_two", 2);
    file.add_dimension("dim_non_existent");  // to be remnamed later

    // groups

    file.add_group("group_non_existent");
    auto group = file.add_group("group");

    // types

    auto type_compound = file.add_type_compound<TypeCompound>("type_compound");
    REQUIRE_OBJ(type_compound.add_compound_field<decltype(TypeCompound::c)>("c", offsetof(TypeCompound, c)), type_compound);
    type_compound.add_compound_field_array<decltype(TypeCompound::i)>("i", offsetof(TypeCompound, i), {3, 2});
    type_compound.add_compound_field<decltype(TypeCompound::d)>("d", offsetof(TypeCompound, d));

    auto type_opaque = file.add_type_opaque("type_opaque", sizeof(TypeOpaque));

    auto type_vlen = file.add_type_vlen<int>("type_vlen");

    auto type_enum = file.add_type_enum<Enum>("type_enum");
    REQUIRE_OBJ(type_enum.add_enum_member<Enum>("E1", Enum::E1), type_enum);
    type_enum.add_enum_member<Enum>("E2", Enum::E2);
    type_enum.add_enum_member<Enum>("E3", Enum::E3);

    // variables

    auto var_float = file.add_variable<float>("var_float", {"dim_unlimited"});
    var_float.set_endianness(NC_ENDIAN_LITTLE);
    var_float.set<float, 1>({123.456f, 789.012f}, {0}, {2});

    auto var_char1 = file.add_variable<unsigned char>("var_char1", {dim_with_size, dim_with_size});
    var_char1.set_chunking({10, 1});
    var_char1.set_checksum_enabled(true);
    var_char1.set_fill<unsigned char>(137);
    var_char1.set_compression(true, 3);
    var_char1.set<unsigned char, 2>({1, 2, 3, 4, 5, 6}, {1, 4}, {2, 3});
    var_char1.set<unsigned char, 2>({1, 2, 3, 4, 5, 6}, {4, 2}, {3, 2}, {2, 3});
    var_char1.set<unsigned char, 2>({7, 255, 8, 255, 255, 255, 255, 255, 255, 9, 255, 0}, {5, 7}, {2, 2}, {1, 2}, {2, 9});

    auto var_char2 = file.add_variable<char>("var_char2", {dim_with_size, dim_with_size});
    var_char2.set_fill<char>(-1);
    {
        char buf = 3;
        var_char2.write<char, 2>(&buf, {2, 2});
    }
    {
        std::vector<char> buf = {1, 2, 3, 4, 5, 6};
        var_char2.write<char, 2>(&buf[0], {1, 4}, {2, 3});
        var_char2.write<char, 2>(&buf[0], {4, 2}, {3, 2}, {2, 3});
    }
    {
        std::vector<char> buf = {7, -127, 8, -127, -127, -127, -127, -127, -127, 9, -127, 0};
        var_char2.write<char, 2>(&buf[0], {5, 7}, {2, 2}, {1, 2}, {2, 9});
    }
    {
        std::vector<char> buf = {1, -127, 2, -127, -127, -127, -127, -127, -127, 3, -127, 4};
        var_char2.write<void, 2>(&buf[0], {5, 6}, {2, 2}, {1, 2}, {2, 9});
    }

    auto var_long_renamed = file.add_variable<long>("var_non_existent", {dim_with_size});
    var_long_renamed.set_chunking({});
    var_long_renamed.unset_fill();
    var_long_renamed.set<long>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    auto var_short = file.add_variable<short>("var_short", std::vector<int>{dim_with_size.id()});
    var_short.set_fill<short>(137);
    var_short.set_default_chunking();
    var_short.set_compression(false, -1);
    var_short.set<short, 1>(0, {0});

    auto var_string = file.add_variable<std::string>("var_string", {dim_with_size});
    var_string.set<std::string>({"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"});

    auto var_compound = file.add_variable("var_compound", type_compound, {"dim_two"});
    var_compound.set<TypeCompound>({{'a', {{1, 2}, {3, 4}, {5, 6}}, 456.789}, {'b', {{9, 8}, {7, 6}, {5, 4}}, 654.321}});

    auto var_vlen = file.add_variable("var_vlen", type_vlen, {"dim_two"});
    std::vector<int> vlen_values = {1, 2, 3};
    var_vlen.set<nc_vlen_t, 1>({vlen_values.size(), &vlen_values[0]}, {0});

    auto var_opaque = file.add_variable("var_opaque", type_opaque, std::vector<int>{dim_with_size.id()});
    var_opaque.set<TypeOpaque, 1>({'a'}, {0});

    auto var_enum = file.add_variable("var_enum", type_enum, {dim_with_size});
    var_enum.set_fill<Enum>(Enum::E1);
    var_enum.set<Enum, 1>({Enum::E2, Enum::E3, Enum::E2}, {1}, {3});
    var_enum.set<Enum, 1>({Enum::E2, Enum::E3, Enum::E2}, {4}, {3}, {2});

    auto group_var_string1 = group.add_variable<std::string>("group_var_string1", {"dim_with_size"});
    group_var_string1.set<std::string>({"a", "X", "X", "d", "e", "f", "g", "h", "i", "j"});
    group_var_string1.set<const char*, 1>("b", {1});
    group_var_string1.set<std::string, 1>("c", {2});

    auto group_var_string2 = group.add_variable<std::string>("group_var_string2", std::vector<int>{dim_with_size.id()});
    group_var_string2.set<std::string>({"X", "X", "X", "X", "X", "f", "X", "X", "X", "j"});
    group_var_string2.set<std::string, 1>({"b", "c", "d"}, {1}, {3});
    group_var_string2.set<std::string, 1>({"e", "g", "i"}, {4}, {3}, {2});
    group_var_string2.set<std::string, 1>({"a", "X", "X", "h", "X", "X"}, {0}, {2}, {7}, {3});

    // attributes

    file.add_attribute("att_string1").set<std::string>("att_value1");
    file.add_attribute("att_string2").set<const char*>("att_value2");
    {
        char buf[11] = {'a', 't', 't', '_', 'v', 'a', 'l', 'u', 'e', '3', '\0'};
        file.add_attribute("att_string3").set<char*>(buf);
    }
    file.add_attribute("att_int").set<int>(123);
    file.add_attribute("att_string_vector1").set<std::string>({"a", "b", "c"});
    file.add_attribute("att_string_vector2").set<const char*>({"a", "b", "c"});
    {
        char buf[3][2] = {{'a', '\0'}, {'b', '\0'}, {'c', '\0'}};
        file.add_attribute("att_string_vector3").set<char*>({buf[0], buf[1], buf[2]});
    }
    file.add_attribute("att_non_existent").set<double>(123.456);

    group.add_attribute("group_att_compound").set<TypeCompound>(TypeCompound{'c', {{2, 4}, {6, 8}, {0, 2}}, 246.802}, type_compound);
    group.add_attribute("group_att_opaque").set<TypeOpaque>(TypeOpaque{'b'}, type_opaque);
    group.add_attribute("group_att_enum").set<Enum>({{Enum::E1}, {Enum::E2}, {Enum::E3}}, type_enum);
    group.add_attribute("group_att_vlen").set<nc_vlen_t>({vlen_values.size(), &vlen_values[0]}, type_vlen);

    var_float.add_attribute("var_att_float_vector").set<float>({1.2f, 3.4f, 5.6f});

    file.sync();
}

TEST_CASE("appending") {
    netCDF::File file("test.nc", 'a');
    file.dimension("dim_non_existent").require().rename("dim_renamed");
    file.group("group_non_existent").require().rename("group_renamed");
    file.variable("var_non_existent").require().rename("var_long_renamed");
    file.attribute("att_non_existent").require().rename("att_renamed");

    REQUIRE_THROWS_WITH_AS(file.add_user_type(netCDF::testing::TestUserType::create()), "Invalid user type class: fake_user_type", netCDF::Exception);
}

TEST_CASE("copying") {
    {
        netCDF::File infile("test.nc", 'r');
        netCDF::File outfile("test_copy.nc", 'w');
        outfile.copy_dimensions(infile);
        REQUIRE_THROWS_WITH_AS(outfile.add_variable<unsigned char>("var_temp1", {"dim_unlimited"}).copy_values(infile.variable("var_float").require()),
                               "Variable type sizes do not match: test_copy.nc:var_temp1 and test.nc:var_float", netCDF::Exception);
        REQUIRE_THROWS_WITH_AS(outfile.add_variable<unsigned char>("var_temp2", {"dim_with_size"}).copy_values(infile.variable("var_char1").require()),
                               "Variable dimension counts do not match: test_copy.nc:var_temp2 and test.nc:var_char1", netCDF::Exception);
        REQUIRE_THROWS_WITH_AS(outfile.add_variable<short>("var_temp3", {"dim_two"}).copy_values(infile.variable("var_short").require()),
                               "Variable sizes do not match: test_copy.nc:var_temp3 and test.nc:var_short", netCDF::Exception);
        auto type_temp = outfile.add_type_compound<TypeOpaque>("type_compound");
        type_temp.add_compound_field<decltype(TypeOpaque::c)>("c", offsetof(TypeOpaque, c));
        REQUIRE_THROWS_WITH_AS(outfile.add_attribute("att_temp").copy_values(infile.group("group").require().attribute("group_att_compound").require()),
                               "Attribute type sizes do not match: test_copy.nc:att_temp and test.nc:group/group_att_compound", netCDF::Exception);
    }

    {
        netCDF::File infile("test.nc", 'r');
        netCDF::File outfile("test_copy.nc", 'w');
        outfile.copy_from(infile, true);
    }
}

TEST_CASE("reading") {
    const std::array<const char*, 2> filenames = {"test.nc", "test_copy.nc"};
    for (std::size_t filename_i = 0; filename_i < filenames.size(); ++filename_i) {
        const auto* filename = filenames[filename_i];
        SUBCASE(filename) {
            netCDF::File file(filename, 'r');

            // dimensions

            if (filename_i == 0) {
                REQUIRE_THROWS_WITH_AS(file.dimension("dim_non_existent").require(), "Dimension not found: test.nc:dim_non_existent", netCDF::Exception);
            }
            REQUIRE_VEC(file.dimensions(), {"dim_unlimited", "dim_with_size", "dim_two", "dim_renamed"});
            REQUIRE(!file.dimension("dim_non_existent"));
            REQUIRE(file.dimension("dim_unlimited").require().is_unlimited());
            REQUIRE(file.dimension("dim_unlimited").require().size() == 2);
            REQUIRE(!file.dimension("dim_with_size").require().is_unlimited());
            REQUIRE(file.dimension("dim_renamed").require().is_unlimited());
            REQUIRE(file.dimension("dim_renamed").require().size() == 0);
            REQUIRE(file.dimension("dim_with_size").require().size() == 10);
            REQUIRE_OBJ(file.dimension("dim_with_size").require().parent(), file);

            // groups

            if (filename_i == 0) {
                REQUIRE_THROWS_WITH_AS(file.group("group_non_existent").require(), "Group not found: test.nc:group_non_existent", netCDF::Exception);
            }
            REQUIRE_VEC(file.groups(), {"group", "group_renamed"});
            REQUIRE(!file.group("group_non_existent"));
            REQUIRE(file.group("group").require().name() == "group");
            REQUIRE(!file.parent());
            REQUIRE_OBJ(file.group("group").require().parent().require(), file);

            // types

            if (filename_i == 0) {
                REQUIRE_THROWS_WITH_AS(file.user_type("type_non_existent").require(), "UserType not found: test.nc:type_non_existent", netCDF::Exception);
            }
            REQUIRE_VEC(file.user_types(), {"type_compound", "type_opaque", "type_vlen", "type_enum"});
            REQUIRE(!file.user_type("type_non_existent"));
            REQUIRE(file.user_type("type_compound").require().bytes_size() == sizeof(TypeCompound));
            REQUIRE(file.user_type("type_compound").require().typeclass() == NC_COMPOUND);
            {
                const auto fields = file.user_type("type_compound").require().compound_fields();
                std::vector<netCDF::UserType::CompoundField> expected = {{NC_CHAR, "c", offsetof(TypeCompound, c), {}},
                                                                         {NC_INT, "i", offsetof(TypeCompound, i), {3, 2}},
                                                                         {NC_DOUBLE, "d", offsetof(TypeCompound, d), {}}};
                REQUIRE(fields.size() == expected.size());
                for (std::size_t i = 0; i < fields.size(); ++i) {
                    REQUIRE(fields[i].type == expected[i].type);
                    REQUIRE(fields[i].name == expected[i].name);
                    REQUIRE(fields[i].offset == expected[i].offset);
                    REQUIRE(fields[i].dimensions == expected[i].dimensions);
                }
            }
            REQUIRE(file.user_type("type_enum").require().memberscount() == 3);
            REQUIRE(file.user_type("type_enum").require().enum_members<Enum>()
                    == std::vector<std::pair<std::string, Enum>>{{"E1", Enum::E1}, {"E2", Enum::E2}, {"E3", Enum::E3}});

            REQUIRE(file.user_type("type_vlen").require().basetype() == NC_INT);
            REQUIRE_OBJ(file.user_type("type_vlen").require().parent(), file);

            // variables

            if (filename_i == 0) {
                REQUIRE_THROWS_WITH_AS(file.variable("var_non_existent").require(), "Variable not found: test.nc:var_non_existent", netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().require_dimensions({"one"}), "Unexpected dimensions: test.nc:var_float",
                                       netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().require_dimensions({"one", "two"}), "Unexpected dimensions: test.nc:var_float",
                                       netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().require_size(200), "Unexpected variable size: test.nc:var_float",
                                       netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().require_type("invalid"), "Unexpected type 'float': test.nc:var_float",
                                       netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().user_type().require(), "UserType not found: test.nc:var_float not of user type",
                                       netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_vlen").require().require_compound<TypeCompound>(2),
                                       "Type 'type_vlen' is not a compound: test.nc:var_vlen", netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_compound").require().require_compound<TypeCompound>(2),
                                       "Unexpected size for type 'type_compound': test.nc:var_compound", netCDF::Exception);
                struct WrongTypeCompound {};
                REQUIRE_THROWS_WITH_AS(file.variable("var_compound").require().require_compound<WrongTypeCompound>(3),
                                       "Unexpected size for type 'type_compound': test.nc:var_compound", netCDF::Exception);
            }
            REQUIRE_VEC(file.variables(), {"var_float", "var_char1", "var_char2", "var_long_renamed", "var_short", "var_string", "var_compound", "var_vlen",
                                           "var_opaque", "var_enum"});
            REQUIRE(!file.variable("var_non_existent"));

            REQUIRE(file.variable("var_float").require().dimension_count() == 1);
            REQUIRE(file.variable("var_float").require().sizes() == std::vector<std::size_t>{2});
            REQUIRE_OBJ(file.variable("var_float").require().require_dimensions({"dim_unlimited"}), file.variable("var_float").require());
            REQUIRE_OBJ(file.variable("var_float").require().require_size(2), file.variable("var_float").require());
            REQUIRE_OBJ(file.variable("var_float").require().require_type("float"), file.variable("var_float").require());
            REQUIRE(file.variable("var_float").require().check_dimensions({"dim_unlimited"}));
            REQUIRE(file.variable("var_float").require().get_endianness() == NC_ENDIAN_LITTLE);
            REQUIRE(!file.variable("var_float").require().get_checksum_enabled());
            REQUIRE(file.variable("var_float").require().get_compression() == std::make_pair<bool, int>(false, -1));
            REQUIRE(!file.variable("var_float").require().check_dimensions({"dim_invalid"}));
            REQUIRE_VEC(file.variable("var_float").require().dimensions(), {"dim_unlimited"});
            REQUIRE(file.variable("var_float").require().size() == 2);
            REQUIRE(file.variable("var_float").require().get<float>() == std::vector<float>{123.456f, 789.012f});

            REQUIRE(file.variable("var_char1").require().dimension_count() == 2);
            REQUIRE(file.variable("var_char1").require().sizes() == std::vector<std::size_t>{10, 10});
            REQUIRE_OBJ(file.variable("var_char1").require().require_dimensions({"dim_with_size", "dim_with_size"}), file.variable("var_char1").require());
            REQUIRE_OBJ(file.variable("var_char1").require().parent(), file);
            REQUIRE(file.variable("var_char1").require().check_dimensions({"dim_with_size", "dim_with_size"}));
            REQUIRE(file.variable("var_char1").require().get_checksum_enabled());
            REQUIRE(file.variable("var_char1").require().get_chunking() == std::vector<std::size_t>{10, 1});
            REQUIRE(file.variable("var_char1").require().get_compression() == std::make_pair<bool, int>(true, 3));
            REQUIRE(file.variable("var_char1").require().get_fill<unsigned char>() == std::make_pair<bool, unsigned char>(true, 137));
            REQUIRE(!file.variable("var_char1").require().check_dimensions({"dim_invalid"}));
            REQUIRE_VEC(file.variable("var_char1").require().dimensions(), {"dim_with_size", "dim_with_size"});
            REQUIRE(file.variable("var_char1").require().size() == 100);
            REQUIRE(file.variable("var_char1").require().size<2>({5, 6}, {2, 2}) == 4);
            REQUIRE(file.variable("var_char1").require().get<unsigned char, 2>({2, 5}) == 5);
            REQUIRE(file.variable("var_char1").require().get<unsigned char, 2>({1, 4}, {2, 3}) == std::vector<unsigned char>{1, 2, 3, 4, 5, 6});
            REQUIRE(file.variable("var_char1").require().get<unsigned char, 2>({4, 2}, {3, 2}, {2, 3}) == std::vector<unsigned char>{1, 2, 3, 4, 5, 6});
            {
                std::vector<unsigned char> buf(12, 255);
                file.variable("var_char1").require().read<unsigned char, 2>(&buf[0], {5, 7}, {2, 2}, {1, 2}, {2, 9});
                REQUIRE(buf == std::vector<unsigned char>{7, 255, 8, 255, 255, 255, 255, 255, 255, 9, 255, 0});
            }
            {
                std::vector<unsigned char> buf(12, 255);
                file.variable("var_char1").require().read<void, 2>(&buf[0], {5, 7}, {2, 2}, {1, 2}, {2, 9});
                REQUIRE(buf == std::vector<unsigned char>{7, 255, 8, 255, 255, 255, 255, 255, 255, 9, 255, 0});
            }
            {
                std::vector<unsigned char> buf(4, 255);
                file.variable("var_char1").require().read<unsigned char, 2>(&buf[0], {5, 7}, {2, 2}, {1, 2});
                REQUIRE(buf == std::vector<unsigned char>{7, 9, 8, 0});
            }
            {
                std::vector<unsigned char> buf(4, 255);
                file.variable("var_char1").require().read<unsigned char, 2>(&buf[0], {1, 4}, {2, 2});
                REQUIRE(buf == std::vector<unsigned char>{1, 2, 4, 5});
            }
            {
                unsigned char buf;
                file.variable("var_char1").require().read<unsigned char, 2>(&buf, {1, 4});
                REQUIRE(buf == 1);
            }

            REQUIRE(file.variable("var_char2").require().get<char, 2>({2, 2}) == 3);
            REQUIRE(file.variable("var_char2").require().get<char, 2>({1, 4}, {2, 3}) == std::vector<char>{1, 2, 3, 4, 5, 6});
            REQUIRE(file.variable("var_char2").require().get<char, 2>({4, 2}, {3, 2}, {2, 3}) == std::vector<char>{1, 2, 3, 4, 5, 6});
            REQUIRE(file.variable("var_char2").require().get<char, 2>({5, 6}, {2, 2}, {1, 2}) == std::vector<char>{1, 3, 2, 4});

            REQUIRE(file.variable("var_long_renamed").require().get_chunking().empty());
            REQUIRE(!file.variable("var_long_renamed").require().get_fill<long>().first);

            REQUIRE(file.variable("var_short").require().get_chunking() == std::vector<std::size_t>{10});
            REQUIRE(file.variable("var_short").require().get_fill<short>() == std::make_pair<bool, short>(true, 137));
            REQUIRE(file.variable("var_short").require().get_compression() == std::make_pair<bool, int>(false, -1));

            REQUIRE(file.variable("var_string").require().get<std::string>() == std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"});
            REQUIRE(file.variable("var_string").require().get<std::string, 1>({3}) == "d");
            REQUIRE(file.variable("var_string").require().get<std::string, 1>({4}, {2}) == std::vector<std::string>{"e", "f"});
            REQUIRE(file.variable("var_string").require().get<std::string, 1>({4}, {3}, {2}) == std::vector<std::string>{"e", "g", "i"});

            REQUIRE(file.variable("var_compound").require().get<TypeCompound>()
                    == std::vector<TypeCompound>{{'a', {{1, 2}, {3, 4}, {5, 6}}, 456.789}, {'b', {{9, 8}, {7, 6}, {5, 4}}, 654.321}});
            REQUIRE_OBJ(file.variable("var_compound").require().require_compound<TypeCompound>(3), file.variable("var_compound").require());

            {
                const auto elem = file.variable("var_vlen").require().get<netCDF::VLenElement<int>, 1>({0});
                REQUIRE(elem.size == 3);
                REQUIRE(elem.data[0] == 1);
                REQUIRE(elem.data[1] == 2);
                REQUIRE(elem.data[2] == 3);
            }

            REQUIRE(file.variable("var_opaque").require().get<TypeOpaque, 1>({0}).c == 'a');

            REQUIRE(file.variable("var_enum").require().get_fill<Enum>() == std::make_pair<bool, Enum>(true, Enum::E1));
            REQUIRE(file.variable("var_enum").require().get<Enum, 1>({1}, {3}) == std::vector<Enum>{Enum::E2, Enum::E3, Enum::E2});
            REQUIRE(file.variable("var_enum").require().get<Enum, 1>({4}, {3}, {2}) == std::vector<Enum>{Enum::E2, Enum::E3, Enum::E2});

            {
                auto group = file.group("group").require();
                REQUIRE_VEC(group.variables(), {"group_var_string1", "group_var_string2"});
                REQUIRE(!file.variable("group_var_string1"));
                REQUIRE(!group.variable("var_string"));
                REQUIRE(!group.variable("group_var_non_existent"));

                REQUIRE(group.variable("group_var_string1").require().get<std::string>()
                        == std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"});
                REQUIRE(group.variable("group_var_string2").require().get<std::string>()
                        == std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"});
            }

            // attributes

            if (filename_i == 0) {
                REQUIRE_THROWS_WITH_AS(file.attribute("att_non_existent").require(), "Attribute not found: test.nc:att_non_existent", netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().attribute("var_att_non_existent").require(),
                                       "Attribute not found: test.nc:var_float/var_att_non_existent", netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.variable("var_float").require().attribute("var_att_float_vector").require().require_type("invalid"),
                                       "Unexpected type 'float': test.nc:var_float/var_att_float_vector", netCDF::Exception);
                REQUIRE_THROWS_WITH_AS(file.attribute("att_int").require().user_type().require(), "UserType not found: test.nc:att_int not of user type",
                                       netCDF::Exception);
            }
            REQUIRE(!file.attribute("att_non_existent"));
            REQUIRE_VEC(file.attributes(), {"att_string1", "att_string2", "att_string3", "att_int", "att_string_vector1", "att_string_vector2",
                                            "att_string_vector3", "att_renamed"});

            REQUIRE(file.attribute("att_string1").require().get_string() == "att_value1");
            REQUIRE(file.attribute("att_string1").require().size() == 11);
            REQUIRE(file.attribute("att_string1").require().is_group_attribute());
            REQUIRE(!file.attribute("att_string1").require().parent_variable());
            REQUIRE_OBJ(file.attribute("att_string1").require().parent_group().require(), file);

            REQUIRE(file.attribute("att_string2").require().get_string() == "att_value2");
            REQUIRE(file.attribute("att_string3").require().get_string() == "att_value3");

            REQUIRE(file.attribute("att_int").require().get<int>() == std::vector<int>{123});
            REQUIRE(file.attribute("att_int").require().size() == 1);
            REQUIRE(file.attribute("att_int").require().type() == NC_INT);

            REQUIRE(file.attribute("att_renamed").require().get<double>() == std::vector<double>{123.456});

            REQUIRE(file.attribute("att_string_vector1").require().get<std::string>() == std::vector<std::string>{"a", "b", "c"});
            REQUIRE(file.attribute("att_string_vector2").require().get<std::string>() == std::vector<std::string>{"a", "b", "c"});
            REQUIRE(file.attribute("att_string_vector3").require().get<std::string>() == std::vector<std::string>{"a", "b", "c"});

            {
                const auto var_float = file.variable("var_float").require();
                REQUIRE(!var_float.attribute("var_att_non_existent"));
                REQUIRE_VEC(var_float.attributes(), {"var_att_float_vector"});
                REQUIRE(var_float.attribute("var_att_float_vector").require().get<float>() == std::vector<float>{1.2f, 3.4f, 5.6f});
                REQUIRE(!var_float.attribute("var_att_float_vector").require().is_group_attribute());
                REQUIRE(!var_float.attribute("var_att_float_vector").require().parent_group());
                REQUIRE_OBJ(var_float.attribute("var_att_float_vector").require().parent_variable().require(), var_float);
                REQUIRE(var_float.attribute("var_att_float_vector").require().type_name() == "float");
                REQUIRE_OBJ(var_float.attribute("var_att_float_vector").require().require_type("float"), var_float.attribute("var_att_float_vector").require());
            }

            {
                auto group = file.group("group").require();
                REQUIRE_VEC(group.attributes(), {"group_att_compound", "group_att_opaque", "group_att_enum", "group_att_vlen"});
                REQUIRE(!file.attribute("grounp_att_compound"));
                REQUIRE(!group.attribute("att_int"));
                REQUIRE(!group.attribute("group_att_non_existent"));
                REQUIRE_OBJ(group.attribute("group_att_compound").require().user_type().require(), file.user_type("type_compound").require());
                REQUIRE(group.attribute("group_att_compound").require().get<TypeCompound>()
                        == std::vector<TypeCompound>{{'c', {{2, 4}, {6, 8}, {0, 2}}, 246.802}});
                REQUIRE(group.attribute("group_att_opaque").require().get<TypeOpaque>() == std::vector<TypeOpaque>{{'b'}});
                REQUIRE(group.attribute("group_att_enum").require().get<Enum>() == std::vector<Enum>{{Enum::E1}, {Enum::E2}, {Enum::E3}});
                {
                    const auto elem = group.attribute("group_att_vlen").require().get<netCDF::VLenElement<int>>();
                    REQUIRE(elem.size() == 1);
                    REQUIRE(elem[0].size == 3);
                    REQUIRE(elem[0].data[0] == 1);
                    REQUIRE(elem[0].data[1] == 2);
                    REQUIRE(elem[0].data[2] == 3);
                }
            }
        }
    }
}
