#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# Top level CMakeLists.txt file (for this software repo)                       #
#                                                                              #
#------------------------------------------------------------------------------#
cmake_minimum_required(VERSION 4.0.2)
project(runtime_diagnostics_project LANGUAGES C) # <== Name your software project

# \/=== Add all interface directories (each w/ a CMakeLists.txt)
add_subdirectory(runtime_diagnostics)
