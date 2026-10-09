cmake_minimum_required(VERSION 3.20)

project(
    RadShift
    VERSION 0.1.0
    LANGUAGES CXX
)

if(NOT UNIX)
    message(
        FATAL_ERROR
        "RadShift currently requires macOS or Linux."
    )
endif()

add_library(
    radshift_core
    src/mapping_resource.cpp
    src/mapped_file.cpp
    src/count_view.cpp
    src/detection_config.cpp
    src/poisson_cusum.cpp
)

target_include_directories(
    radshift_core
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(
    radshift_core
    PUBLIC
    cxx_std_20
)

set_target_properties(
    radshift_core
    PROPERTIES
    CXX_EXTENSIONS OFF
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        radshift_core
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_executable(
    radshift
    src/main.cpp
)

target_link_libraries(
    radshift
    PRIVATE
    radshift_core
)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        test_poisson_cusum
        tests/test_poisson_cusum.cpp
    )

    target_link_libraries(
        test_poisson_cusum
        PRIVATE
        radshift_core
    )

    add_test(
        NAME poisson_cusum
        COMMAND test_poisson_cusum
    )

    add_executable(
        test_storage
        tests/tests_storage.cpp
    )

    target_link_libraries(
        test_storage
        PRIVATE
        radshift_core
    )

    add_test(
        NAME storage
        COMMAND test_storage
    )
endif()
