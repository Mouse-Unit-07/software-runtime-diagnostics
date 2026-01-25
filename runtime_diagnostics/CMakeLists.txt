#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# CMakeLists.txt file for hello_world source files                             #
#                                                                              #
#------------------------------------------------------------------------------#
# \/=== Add all .c files to your build target
add_library(runtime_diagnostics_lib STATIC # <== Define this interface's build target
    ${CMAKE_CURRENT_LIST_DIR}/runtime_diagnostics.c
)

# \/=== Add all directories w/ header files
target_include_directories(runtime_diagnostics_lib PUBLIC # <=== Rename to your build target
    ${CMAKE_CURRENT_LIST_DIR}
)
