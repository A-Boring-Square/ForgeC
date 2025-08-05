#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #include <windows.h>
    #include <io.h>
    #define PLATFORM "WIN"
    #define PATH_SEPARATOR '\\'
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #define PLATFORM "UNIX"
    #define PATH_SEPARATOR '/'
#endif

// ASCII logo banner printed on startup.
#define LOGO \
"8888888888                                 .d8888b.  \n" \
"888                                       d88P  Y88b\n" \
"888                                       888    888\n" \
"8888888  .d88b.  888d888 .d88b.   .d88b.  888       \n" \
"888     d88\"\"88b 888P\"  d88P\"88b d8P  Y8b 888       \n" \
"888     888  888 888    888  888 88888888 888    888\n" \
"888     Y88..88P 888    Y88b 888 Y8b.     Y88b  d88P\n" \
"888      \"Y88P\"  888     \"Y88888  \"Y8888   \"Y8888P\" \n" \
"                             888                    \n" \
"                        Y8b d88P                    \n" \
"                         \"Y88P\"                     \n"\
"C/C++ Build System\n\n\n"

// Types of errors that might occur
typedef enum {
    NONE,               // No error
    COMPILER_ERROR,     // Compiler failed (e.g., bad flags, missing file)
    BUILD_SYSTEM_ERROR  // Internal failure in the build system
} error_t;

// Configuration struct for the build environment
typedef struct {
    const char* compiler;       // Compiler name (e.g., gcc, clang, cl)
    const char* compiler_cmd;   // Full command string after setup
    const char* source_dir;     // Folder with source code
    const char* main_file;
    const char** include_dirs;  // NULL-terminated array of include paths
    const char** args_buffer;   // Internal args list for processing
    unsigned int arg_count;
} build_env_t;

// Clears the console depending on the platform
void forgec_clear_cmd() {
    if (strcmp(PLATFORM, "WIN") == 0) {
        system("cls");   // Windows clear
    } else {
        system("clear"); // Unix clear
    }
}

// DJB2-based string hash function with a random seed
// This can be used to generate IDs or cache keys for builds
unsigned long forgec_hash_string(const char *str) {
    unsigned long hash = rand(); // Use random starting value for more entropy
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

// Initialize build environment
build_env_t* forgec_init() {
    srand(time(NULL));      // Seed RNG for use in hashing
    forgec_clear_cmd();     // Clear terminal
    printf(LOGO);           // Show startup banner
    build_env_t* build_env = (build_env_t*)malloc(sizeof(build_env_t));
    build_env->args_buffer = NULL;
    build_env->arg_count = 0;
    build_env->compiler_cmd = NULL;
    build_env->compiler = NULL;
    build_env->source_dir = NULL;
    build_env->main_file = NULL;
    build_env->include_dirs = NULL;
    return build_env;
}

void forgec_cleanup(build_env_t* build_enviroment) {
    if (build_enviroment->args_buffer) free((void*)build_enviroment->args_buffer);
    free(build_enviroment);
}

void forgec_select_compiler(build_env_t* build_enviroment, const char* compiler) {
    build_enviroment->compiler = compiler;
}

void forgec_add_compiler_arg(build_env_t* build_enviroment, const char* arg) {
    unsigned int new_size = build_enviroment->arg_count + 1;
    const char** new_buffer = realloc(build_enviroment->args_buffer, new_size * sizeof(char*));
    if (!new_buffer) {
        fprintf(stderr, "Failed to allocate memory for compiler args\n");
        exit(EXIT_FAILURE);
    }
    build_enviroment->args_buffer = new_buffer;
    build_enviroment->args_buffer[build_enviroment->arg_count] = arg;
    build_enviroment->arg_count = new_size;
}

// Create Build directory if missing
void forgec_create_build_dir() {
#if defined(_WIN32) || defined(_WIN64)
    system("if not exist Build mkdir Build");
#else
    system("mkdir -p Build");
#endif
}

// Helper: copy one file (binary safe)
int forgec_copy_file(const char* src, const char* dest) {
    FILE* fsrc = fopen(src, "rb");
    if (!fsrc) return 0;
    FILE* fdest = fopen(dest, "wb");
    if (!fdest) {
        fclose(fsrc);
        return 0;
    }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fsrc)) > 0) {
        fwrite(buffer, 1, n, fdest);
    }

    fclose(fsrc);
    fclose(fdest);
    return 1;
}

// Helper: recursively copy directory contents to dest dir
int forgec_copy_dir(const char* src_dir, const char* dest_dir) {
#if defined(_WIN32) || defined(_WIN64)
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", src_dir);

    if (_mkdir(dest_dir) == -1) {
        // Might already exist; ignore
    }

    hFind = FindFirstFile(search_path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        char src_path[MAX_PATH];
        char dst_path[MAX_PATH];
        snprintf(src_path, MAX_PATH, "%s\\%s", src_dir, findFileData.cFileName);
        snprintf(dst_path, MAX_PATH, "%s\\%s", dest_dir, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!forgec_copy_dir(src_path, dst_path)) {
                FindClose(hFind);
                return 0;
            }
        } else {
            if (!forgec_copy_file(src_path, dst_path)) {
                FindClose(hFind);
                return 0;
            }
        }
    } while (FindNextFile(hFind, &findFileData));

    FindClose(hFind);
    return 1;
#else
    DIR* dir = opendir(src_dir);
    if (!dir) return 0;

    struct stat st = {0};
    if (stat(dest_dir, &st) == -1) {
        mkdir(dest_dir, 0755);
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src_path[1024];
        char dst_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dest_dir, entry->d_name);

        struct stat entry_stat;
        if (stat(src_path, &entry_stat) == -1) {
            closedir(dir);
            return 0;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            if (!forgec_copy_dir(src_path, dst_path)) {
                closedir(dir);
                return 0;
            }
        } else {
            if (!forgec_copy_file(src_path, dst_path)) {
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);
    return 1;
#endif
}

// Build the compiler command string: "compiler args... -o output_path"
void forgec_build_command(build_env_t* env, const char* output_path) {
    static char cmd[8192];
    size_t offset = 0;

    offset += snprintf(cmd + offset, sizeof(cmd) - offset, "%s", env->compiler);

    for (unsigned int i = 0; i < env->arg_count; i++) {
        offset += snprintf(cmd + offset, sizeof(cmd) - offset, " %s", env->args_buffer[i]);
    }

    offset += snprintf(cmd + offset, sizeof(cmd) - offset, " -o %s", output_path);

    env->compiler_cmd = cmd;
}

// Build executable: puts output in Build/
error_t forgec_build_executable(build_env_t* env, const char* output_name) {
    forgec_create_build_dir();

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "Build%c%s", PATH_SEPARATOR, output_name);

    forgec_build_command(env, output_path);

    printf("Building executable: %s\n", env->compiler_cmd);
    int ret = system(env->compiler_cmd);
    return ret == 0 ? NONE : COMPILER_ERROR;
}

// Build shared library (.dll/.so)
error_t forgec_build_shared(build_env_t* env, const char* output_name) {
    forgec_create_build_dir();

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "Build%c%s", PATH_SEPARATOR, output_name);

    // Add shared library flag based on platform/compiler
#if defined(_WIN32) || defined(_WIN64)
    forgec_add_compiler_arg(env, "-shared");
#else
    forgec_add_compiler_arg(env, "-shared");
#endif

    forgec_build_command(env, output_path);

    printf("Building shared library: %s\n", env->compiler_cmd);
    int ret = system(env->compiler_cmd);
    return ret == 0 ? NONE : COMPILER_ERROR;
}

// Build static library (.lib/.a)
error_t forgec_build_static(build_env_t* env, const char* output_name) {
    forgec_create_build_dir();

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "Build%c%s", PATH_SEPARATOR, output_name);

#if defined(_WIN32) || defined(_WIN64)
    // Use lib.exe or equivalent
    // This example assumes gcc, so use 'ar' on Unix-like or 'lib' on Windows
    char ar_cmd[2048];
    snprintf(ar_cmd, sizeof(ar_cmd), "ar rcs %s *.o", output_path);
    printf("Building static library: %s\n", ar_cmd);
    int ret = system(ar_cmd);
    return ret == 0 ? NONE : COMPILER_ERROR;
#else
    char ar_cmd[2048];
    snprintf(ar_cmd, sizeof(ar_cmd), "ar rcs %s *.o", output_path);
    printf("Building static library: %s\n", ar_cmd);
    int ret = system(ar_cmd);
    return ret == 0 ? NONE : COMPILER_ERROR;
#endif
}

// Build object file (.o or .obj)
error_t forgec_build_obj(build_env_t* env, const char* output_name) {
    forgec_create_build_dir();

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "Build%c%s", PATH_SEPARATOR, output_name);

    forgec_add_compiler_arg(env, "-c"); // Compile only flag

    forgec_build_command(env, output_path);

    printf("Building object file: %s\n", env->compiler_cmd);
    int ret = system(env->compiler_cmd);
    return ret == 0 ? NONE : COMPILER_ERROR;
}
