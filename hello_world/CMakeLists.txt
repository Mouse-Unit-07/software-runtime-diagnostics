#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# CMakeLists.txt file for hello_world source files                             #
#                                                                              #
#------------------------------------------------------------------------------#
# \/=== Add all .c files to your build target
add_library(hello_world_lib STATIC # <== Define this interface's build target
    ${CMAKE_CURRENT_LIST_DIR}/print_hello_world.c
)

# \/=== Add all directories w/ header files
target_include_directories(hello_world_lib PUBLIC # <=== Rename to your build target
    ${CMAKE_CURRENT_LIST_DIR}
)

# \/=== Link to libraries in other GitHub projects
# Example dependency on Hello World project (check out Repeat Hello World project):
#target_link_libraries(repeat_hello_world_lib PUBLIC
#    hello_world_lib
#)