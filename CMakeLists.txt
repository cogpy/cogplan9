cmake_minimum_required(VERSION 3.10...3.31)
project(cogpy-hpp LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Portable C cognitive library ─────────────────────────────────────────────
add_library(plan9cog STATIC
    src/atomspace.c
    src/pln.c
    src/plan9cog.c
)
target_include_directories(plan9cog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# Link math library (not needed on MSVC where it is part of the CRT)
if(NOT MSVC)
    target_link_libraries(plan9cog PUBLIC m)
endif()

# ── C++ header-only library ───────────────────────────────────────────────────
add_library(cog INTERFACE)
target_include_directories(cog INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── Testing ───────────────────────────────────────────────────────────────────
enable_testing()

# Helper macro: add a C test executable under tests/<subdir>/<name>
macro(add_c_test subdir name)
    add_executable(${name}
        tests/${subdir}/${name}.c
    )
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(${name} PRIVATE plan9cog)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/tests/${subdir}
    )
    add_test(NAME ${subdir}/${name}
             COMMAND ${CMAKE_BINARY_DIR}/tests/${subdir}/${name})
endmacro()

# ── Unit tests ────────────────────────────────────────────────────────────────
add_c_test(unit test_atomspace)
add_c_test(unit test_pln)
add_c_test(unit test_atomspace_comprehensive)
add_c_test(unit test_pln_comprehensive)
add_c_test(unit test_edge_cases)
add_c_test(unit test_stress)

# ── API tests ─────────────────────────────────────────────────────────────────
add_c_test(api test_plan9cog_api)

# ── Integration tests (Phase 2E: Full Validation) ─────────────────────────────
add_c_test(integration test_integration)

# ── C++ header-only test (legacy) ─────────────────────────────────────────────
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_all.cpp)
    add_executable(test_all test/test_all.cpp)
    target_link_libraries(test_all PRIVATE cog)
    add_test(NAME cog_cpp_tests COMMAND test_all)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(DIRECTORY include/cog       DESTINATION include)
install(DIRECTORY include/plan9cog  DESTINATION include)
