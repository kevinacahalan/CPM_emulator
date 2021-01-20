#ifdef __linux__


#include "portable.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INVALID_FILE_HANDLE -1
#define HANDLE int

/*
void *map_a_new_file_private(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = open(filename, O_RDONLY | O_CREAT, 0666);
        ftruncate(fh, n_bytes);
    }

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);

    // No idea: safe and proper to close file on Windows?
    close(fh);

    return whole_region;
}

void *map_a_new_file_readonly(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = open(filename, O_RDONLY | O_CREAT, 0666);
        ftruncate(fh, n_bytes);
    }

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ, MAP_PRIVATE | MAP_FIXED, fh, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ, MAP_PRIVATE | MAP_FIXED, fh, 0);

    // No idea: safe and proper to close file on Windows?
    close(fh);

    return whole_region;
}
*/

void *map_a_new_file_shared(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    int mmap_flags;
    if(filename){
        fh = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        ftruncate(fh, n_bytes);
		mmap_flags = MAP_SHARED | MAP_FIXED | MAP_FILE;
    }else{
		mmap_flags = MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS;
		printf("bad, not done correctly\n");
		exit(1);
	}

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ | PROT_WRITE, mmap_flags, section, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ | PROT_WRITE, mmap_flags, section, 0);
    close(fh);

    return whole_region;
}

/*
void *map_an_existing_private(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = open(filename, O_RDONLY, 0666);
        ftruncate(fh, n_bytes);
    }

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);

    // No idea: safe and proper to close file on Windows?
    close(fh);

    return whole_region;
}

void *map_an_existing_readonly(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = open(filename, O_RDONLY, 0666);
        ftruncate(fh, n_bytes);
    }

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);

    // No idea: safe and proper to close file on Windows?
    close(fh);

    return whole_region;
}

void *map_an_existing_shared(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = open(filename, O_RDWR | O_CREAT, 0666);
        ftruncate(fh, n_bytes);
    }

    // char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    char *whole_region = mmap(NULL, 2 * n_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, INVALID_FILE_HANDLE, 0);
    if (whole_region == MAP_FAILED){
        puts("failed, 34");
        exit(34);
    }

    // int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    HANDLE section = fh;

    mmap(whole_region,           n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);
    mmap(whole_region + n_bytes, n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fh, 0);

    // No idea: safe and proper to close file on Windows?
    close(fh);

    return whole_region;
}

void map_jit_buffers(char **read_write, const char **read_exec, size_t n_bytes){
    char *mapping = mmap(0, sz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, 0, 0);
    if(mapping == MAP_FAILED){
        *read_exec = NULL;
        *read_write = NULL;
    }

    *read_exec = mapping;
    *read_write = mapping;
}
*/

void free_jit_buffers(char *read_write, const char *read_exec, size_t n_bytes){
    (void)read_exec;
    munmap(read_write, n_bytes);
}



#endif // for __linux__
