#ifndef PORTABLE_H
#define PORTABLE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void *map_a_new_file_private(const char *restrict const filename, size_t n_bytes);
void *map_a_new_file_readonly(const char *restrict const filename, size_t n_bytes);
void *map_a_new_file_shared(const char *restrict const filename, size_t n_bytes);

void *map_an_existing_private(const char *restrict const filename, size_t *n_bytes);
void *map_an_existing_readonly(const char *restrict const filename, size_t *n_bytes);
void *map_an_existing_shared(const char *restrict const filename, size_t *n_bytes);

void map_jit_buffers(void *read_write, void *read_exsc, size_t n_bytes);

void free_jit_buffers(char *read_write, const char *read_exec, size_t n_bytes);


#ifdef __cplusplus
}
#endif
#endif

