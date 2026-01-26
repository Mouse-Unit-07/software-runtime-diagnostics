#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# CMakeLists.txt file for hello_world source files                             #
#                                                                              #
#------------------------------------------------------------------------------#
add_library(runtime_diagnostics_lib STATIC
    ${CMAKE_CURRENT_LIST_DIR}/runtime_diagnostics.c
)

target_include_directories(runtime_diagnostics_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
