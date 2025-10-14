ForgeC Build System
===================

Overview
--------

ForgeC is a lightweight, cross-platform C/C++ build system designed to simplify compilation and library management without relying on complex Makefiles. It works on both Windows and Unix-like systems.

### Key Features:

*   Cross-platform: Windows and Unix support
*   Multiple compiler support: `gcc`, `clang`, `cl`, `cc`, `c++`
*   Automatic creation of `Build/` directory
*   Builds executables, shared libraries, static libraries, and object files
*   Handles compiler flags, include directories, and source files automatically
*   Minimal setup for small-to-medium projects

**Purpose:**  
ForgeC removes the need for platform-specific scripts and reduces boilerplate in build processes, making compilation consistent and repeatable.

Usage
-----

Include the `ForgeC` header in your project and call its API functions to configure and trigger builds.

    #include "forgec.h"
    
    int main() {
        build_env_t* env = forgec_init();
        forgec_select_compiler(env, "gcc");
        forgec_add_compiler_arg(env, "-Wall");
        forgec_add_compiler_arg(env, "-std=c11");
        forgec_add_include_dir(env, "include");
        env->source_dir = "src";
    
        forgec_build_executable(env, "MyApp", 1);
    
        forgec_cleanup(env);
        return 0;
    }
    

API Reference
-------------

Function

Description

`build_env_t* forgec_init()`

Initializes the build environment and prints the ForgeC logo.

`void forgec_cleanup(build_env_t* env)`

Frees resources associated with the build environment.

`void forgec_select_compiler(build_env_t* env, const char* compiler)`

Selects the compiler (`gcc`, `clang`, `cl`, etc.).

`void forgec_add_compiler_arg(build_env_t* env, const char* arg)`

Adds a compiler flag or argument.

`void forgec_add_include_dir(build_env_t* env, const char* path)`

Adds an include directory (`-I`) to the compiler command.

`void forgec_create_build_dir()`

Creates `Build/` directory if it doesn’t exist.

`void forgec_add_source_files_from_dir(build_env_t* env, const char* dir)`

Adds all `.c` files from a source directory to the build.

`void forgec_apply_build_mode(build_env_t* env, int is_debug)`

Adds debug (`-g`, `-O0`) or release (`-O2`) flags.

`error_t forgec_build_executable(build_env_t* env, const char* output_name, int is_debug)`

Compiles all source files into an executable.

`error_t forgec_build_shared(build_env_t* env, const char* output_name, int is_debug)`

Builds a shared library (`.dll`/`.so`).

`error_t forgec_build_obj(build_env_t* env, const char* output_name, int is_debug)`

Builds object files (`.o`/`.obj`).

`error_t forgec_build_static(build_env_t* env, const char* output_name, int is_debug)`

Builds a static library (`.a`/`.lib`).

Platform Notes
--------------

*   Windows paths use `\` and rely on system commands `mkdir` or `if not exist`.
*   Unix-like systems use `/` and `mkdir` with permissions `0755`.
*   Compiler commands are automatically constructed and printed for transparency.

License
-------

This project is open-source. Use and modify freely with attribution.