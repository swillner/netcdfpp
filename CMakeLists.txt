cmake_minimum_required(VERSION 3.8)

project(netcdfpp)

set(CMAKE_BUILD_TYPE Debug)

add_executable(test_netcdfpp tests/test_netcdfpp.cpp)
target_include_directories(test_netcdfpp PRIVATE lib/doctest/doctest)
target_compile_features(test_netcdfpp PUBLIC cxx_std_14)
target_compile_options(test_netcdfpp PRIVATE -Wall -pedantic -Wextra -Wno-reorder --coverage)
target_link_options(test_netcdfpp PRIVATE --coverage)

add_executable(test_include_self_contained tests/test_include_self_contained.cpp)
target_compile_features(test_include_self_contained PUBLIC cxx_std_14)

include(netcdfpp.cmake)
include_netcdfpp(test_netcdfpp)
include_netcdfpp(test_include_self_contained)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif()

add_custom_target(run_main_test
  COMMAND test_netcdfpp
  BYPRODUCTS test.nc test_copy.nc
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS test_netcdfpp)

add_custom_target(convert_test_files
  COMMAND ncdump test.nc > test.cdl
  COMMAND ncdump test_copy.nc > test_copy.cdl
  BYPRODUCTS test.cdl test_copy.cdl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS run_main_test test.nc test_copy.nc)

add_custom_target(test_generated_files
  COMMAND bash -c "diff ${CMAKE_SOURCE_DIR}/tests/test.cdl test.cdl"
  COMMAND bash -c "diff -I '^netcdf test.* {$$' ${CMAKE_SOURCE_DIR}/tests/test.cdl test_copy.cdl"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS convert_test_files test.cdl test_copy.cdl)

add_custom_target(test
  DEPENDS test_generated_files test_include_self_contained)

add_custom_target(coverage
  COMMAND lcov --quiet --capture --include "**/include/netcdfpp.h" --directory . --output-file coverage.info
  COMMAND genhtml coverage.info --output-directory coverage
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS test)
