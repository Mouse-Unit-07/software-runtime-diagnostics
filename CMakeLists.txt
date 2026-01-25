#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# Top level CMakeLists.txt file (for this software repo)                       #
#                                                                              #
#------------------------------------------------------------------------------#
cmake_minimum_required(VERSION 4.0.2)
project(hello_world_lib LANGUAGES C) # <== Name your software project

include(FetchContent)

# \/=== Add dependencies to other GitHub repos
# Example dependency on Hello World project (check out Repeat Hello World project):
#FetchContent_Declare(
#    print_hello_world
#    GIT_REPOSITORY https://github.com/Mouse-Unit-07/experiment-software-hello-world
#    GIT_TAG main
#)
# FetchContent_MakeAvailable(print_hello_world)

# \/=== Add all interface directories (each w/ a CMakeLists.txt)
add_subdirectory(hello_world)
