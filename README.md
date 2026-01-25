# Hello World Repo

- A simple repo that defines a "Hello World" interface
- This repo has the required structure (CMakeLists.txt files, etc) to be compatible w/ our uniform development environment
- Refer to the `software-repeat-hello-world` repository for an example of a project that has a dependency on another micromouse software library/project

## Creating Your Own Software Repository

- When creating your own software repo, you can copy this repo and modify:
- `software-hello-world/`
  - Top level project directory- rename it to whatever you need (`software-your-repo-name`)
  - `main.c`
    - Just a file for testing code on hardware- customize as needed
  - `CMakeLists.txt`
    - Top level CMakeLists.txt file (for this software repo)
    - Refer to the file for details on what to change
    - Your project and build target names can be identical
  - `hello-world/`
    - An interface in your software repo
    - You can define as many interfaces as you want, but they each need a CMakeLists.txt file and `tests/` folder
    - `print_hello_world.c` & `print_hello_world.h`
      - You can define whatever interface files you want
    - `CMakeLists.txt`
      - Interface level CMakeLists.txt file
      - Refer to the file for details on what to change
    - `tests/`
      - Test directory for your interface
      - All .cpp CppUTest files for your interface are defined here
      - `CMakeLists.txt`
        - Test level CMakeLists.txt file
        - Refer to the file for details on what to change
      - `test_print_hello_world.cpp`
        - CppUTest file for your interface
