#--------------------------------- FILE INFO ----------------------------------#
# Filename           : CMakeLists.txt                                          #
#                                                                              #
# Top level CMakeLists.txt file (for this software repo)                       #
#                                                                              #
#------------------------------------------------------------------------------#
cmake_minimum_required(VERSION 4.0.2)
project(runtime_diagnostics_project LANGUAGES C)

add_subdirectory(runtime_diagnostics)

if(TARGET_WINDOWS AND SUPPORTS_WINDOWS)
    if(ENABLE_RUNTIME_DIAGNOSTICS_TESTS)
        add_subdirectory(runtime_diagnostics/tests)
    endif()
endif()
