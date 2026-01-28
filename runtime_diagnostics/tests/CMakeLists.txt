#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# CMakeLists.txt file for runtime_diagnostics test files                       #
#                                                                              #
#------------------------------------------------------------------------------#
set(ALL_TEST_SOURCES
    ${ALL_TEST_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_diagnostics.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime_diagnostics/runtime_diagnostics.c
    PARENT_SCOPE
)

add_executable(test_runtime_diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_diagnostics.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
)

target_include_directories(test_runtime_diagnostics PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(test_runtime_diagnostics PRIVATE
    runtime_diagnostics_lib
    CppUTest::CppUTest
    CppUTestExt
)

add_test(NAME test_runtime_diagnostics COMMAND test_runtime_diagnostics)
