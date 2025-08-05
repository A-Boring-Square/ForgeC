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
"                         \"Y88P\"                     \n" \
"C/C++ Build System\n\n\n"

typedef enum {
    NONE,
    COMPILER_ERROR,
    BUILD_SYSTEM_ERROR
} error_t;

typedef struct {
    const char* compiler;
    const char* compiler_cmd;
    const char* source_dir;
    const char* main_file;
    const char** include_dirs;
    const char** args_buffer;
    unsigned int arg_count;
} build_env_t;

void forgec_clear_cmd() {
    if (strcmp(PLATFORM, "WIN") == 0)
        system("cls");
    else
        system("clear");
}

build_env_t* forgec_init() {
    srand(time(NULL));
    forgec_clear_cmd();
    printf(LOGO);
    build_env_t* env = (build_env_t*)calloc(1, sizeof(build_env_t));
    return env;
}

void forgec_cleanup(build_env_t* env) {
    if (env->args_buffer) free((void*)env->args_buffer);
    free(env);
}

void forgec_select_compiler(build_env_t* env, const char* compiler) {
    env->compiler = compiler;
}

void forgec_add_compiler_arg(build_env_t* env, const char* arg) {
    unsigned int new_size = env->arg_count + 1;
    const char** new_buf = realloc(env->args_buffer, new_size * sizeof(char*));
    if (!new_buf) {
        fprintf(stderr, "Compiler arg allocation failed\n");
        exit(EXIT_FAILURE);
    }
    env->args_buffer = new_buf;
    env->args_buffer[env->arg_count] = strdup(arg); // deep copy
    env->arg_count = new_size;
}

void forgec_add_include_dir(build_env_t* env, const char* path) {
    char inc_arg[512];
    snprintf(inc_arg, sizeof(inc_arg), "-I%s", path);
    forgec_add_compiler_arg(env, strdup(inc_arg));
}


void forgec_create_build_dir() {
#if defined(_WIN32) || defined(_WIN64)
    system("if not exist Build mkdir Build");
#else
    mkdir("Build", 0755);
#endif
}

void forgec_add_source_files_from_dir(build_env_t* env, const char* dir) {
#if defined(_WIN32) || defined(_WIN64)
    char search[512];
    snprintf(search, sizeof(search), "%s\\*.c", dir);
    WIN32_FIND_DATA data;
    HANDLE h = FindFirstFile(search, &data);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir, data.cFileName);
        forgec_add_compiler_arg(env, _strdup(full_path));
    } while (FindNextFile(h, &data));

    FindClose(h);
#else
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* entry;
    while ((entry = readdir(d))) {
        if (strstr(entry->d_name, ".c")) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
            forgec_add_compiler_arg(env, path);
        }
    }
    closedir(d);
#endif
}

void forgec_apply_build_mode(build_env_t* env, int is_debug) {
    if (is_debug) {
        forgec_add_compiler_arg(env, "-g");
        forgec_add_compiler_arg(env, "-O0");
    } else {
        forgec_add_compiler_arg(env, "-O2");
    }
}

void forgec_build_command(build_env_t* env, const char* output_path) {
    static char cmd[8192];
    size_t off = 0;
    off += snprintf(cmd + off, sizeof(cmd) - off, "%s", env->compiler);

    for (unsigned int i = 0; i < env->arg_count; ++i)
        off += snprintf(cmd + off, sizeof(cmd) - off, " %s", env->args_buffer[i]);

    off += snprintf(cmd + off, sizeof(cmd) - off, " -o %s", output_path);
    env->compiler_cmd = cmd;
}

error_t forgec_build_executable(build_env_t* env, const char* output_name, int is_debug) {
    forgec_create_build_dir();
    forgec_apply_build_mode(env, is_debug);
    if (env->source_dir) forgec_add_source_files_from_dir(env, env->source_dir);

    char out[512];
    snprintf(out, sizeof(out), "Build%c%s", PATH_SEPARATOR, output_name);
    forgec_build_command(env, out);

    printf("Building executable: %s\n", env->compiler_cmd);
    return system(env->compiler_cmd) == 0 ? NONE : COMPILER_ERROR;
}

error_t forgec_build_shared(build_env_t* env, const char* output_name, int is_debug) {
    forgec_create_build_dir();
    forgec_apply_build_mode(env, is_debug);
    forgec_add_compiler_arg(env, "-shared");
    if (env->source_dir) forgec_add_source_files_from_dir(env, env->source_dir);

    char out[512];
    snprintf(out, sizeof(out), "Build%c%s", PATH_SEPARATOR, output_name);
    forgec_build_command(env, out);

    printf("Building shared library: %s\n", env->compiler_cmd);
    return system(env->compiler_cmd) == 0 ? NONE : COMPILER_ERROR;
}

error_t forgec_build_obj(build_env_t* env, const char* output_name, int is_debug) {
    forgec_create_build_dir();
    forgec_apply_build_mode(env, is_debug);
    forgec_add_compiler_arg(env, "-c");
    if (env->source_dir) forgec_add_source_files_from_dir(env, env->source_dir);

    char out[512];
    snprintf(out, sizeof(out), "Build%c%s", PATH_SEPARATOR, output_name);
    forgec_build_command(env, out);

    printf("Building object file: %s\n", env->compiler_cmd);
    return system(env->compiler_cmd) == 0 ? NONE : COMPILER_ERROR;
}

error_t forgec_build_static(build_env_t* env, const char* output_name, int is_debug) {
    forgec_create_build_dir();
    forgec_apply_build_mode(env, is_debug);
    if (env->source_dir) forgec_add_source_files_from_dir(env, env->source_dir);

    // Compile to .o files
    for (unsigned int i = 0; i < env->arg_count; ++i) {
        const char* src = env->args_buffer[i];
        char obj_out[512];
        snprintf(obj_out, sizeof(obj_out), "Build%cfile%d.o", PATH_SEPARATOR, i);
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "%s -c %s -o %s", env->compiler, src, obj_out);
        printf("Compiling: %s\n", cmd);
        if (system(cmd) != 0) return COMPILER_ERROR;
    }

    // Archive .o files
    char archive_cmd[1024];
    snprintf(archive_cmd, sizeof(archive_cmd), "ar rcs Build%c%s Build%c*.o", PATH_SEPARATOR, output_name, PATH_SEPARATOR);
    printf("Creating static library: %s\n", archive_cmd);
    return system(archive_cmd) == 0 ? NONE : COMPILER_ERROR;
}
