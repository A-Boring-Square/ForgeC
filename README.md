ForgeC Build System
===================

Overview
--------

ForgeC is a lightweight, cross-platform build system inspired by `make`, designed specifically for C and C++ projects.

**Features:**

*   Supports multiple compilers (gcc, clang, MSVC, cc, c++)
*   Manages compiler flags and arguments easily
*   Automatically creates a clean `Build/` directory for outputs
*   Copies source and resource files to the build directory
*   Builds executables, shared libraries, static libraries, and object files
*   Abstracts platform differences (Windows vs Unix) for consistent behavior

**Purpose:**

Simplifies the build process without complex Makefiles or platform-specific scripts, making it ideal for small to medium-sized projects that need simple, repeatable, and cross-platform builds.

Usage
-----

Call the API functions in your build scripts to select compiler, add arguments, set source directory, and trigger builds.

API Functions
-------------

    // Initializes the build system and returns a build environment handle
    build_env_t* forgec_init();
    
    // Cleans up build environment and frees resources
    void forgec_cleanup(build_env_t* build_env);
    
    // Selects compiler by name (e.g., "gcc", "clang", "cl", "cc", "c++")
    void forgec_select_compiler(build_env_t* build_env, const char* compiler);
    
    // Adds a compiler argument flag (e.g., "-Wall", "-O2")
    void forgec_add_compiler_arg(build_env_t* build_env, const char* arg);
    
    // Sets source directory path
    void forgec_set_src_dir(build_env_t* build_env, const char* path);
    
    // Sets the main source file to compile (relative to source directory)
    void forgec_set_main_file(build_env_t* build_env, const char* filename);
    
    // Builds executable, placing output into Build/ directory
    error_t forgec_build_executable(build_env_t* build_env);
    
    // Builds shared library (.dll/.so)
    error_t forgec_build_shared(build_env_t* build_env);
    
    // Builds static library (.lib/.a)
    error_t forgec_build_static(build_env_t* build_env);
    
    // Builds object files (.o/.obj)
    error_t forgec_build_object(build_env_t* build_env);