#ifdef _WIN32

#include "portable.h"
// #include <windows.h>
#include <memoryapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileapi.h>
#include <winbase.h>


/*
cfa_flags = writing_file ? GENERIC_READ|GENERIC_WRITE : GENERIC_READ
cfm_flags = writing_ram  ? PAGE_READWRITE : PAGE_READ
cnf_flags = overwrite ? CREATE_ALWAYS : OPEN_EXISTING

*/






void *map_a_new_file_private(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);

    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;

}

void *map_a_new_file_readonly(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READ, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;
}

void *map_a_new_file_shared(const char *restrict const filename, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;
}

void *map_an_existing_private(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;
}

void *map_an_existing_readonly(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READ, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;
}

void *map_an_existing_shared(const char *restrict const filename, size_t *n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;
    if(filename){
        fh = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointerEx(fh, n_bytes, NULL, FILE_BEGIN);
        SetEndOfFile(fh);
    }

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    return whole_region;
}

void map_jit_buffers(void **read_write, void **read_exec, size_t n_bytes){
    HANDLE fh = INVALID_FILE_HANDLE;

    char *whole_region = VirtualAlloc2(NULL, NULL, 2 * n_bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (whole_region == NULL){
        puts("failed, 34");
        exit(34);
    }

    int thing = VirtualFree(whole_region, n_btyes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    HANDLE section = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, n_btyes, NULL);
    CloseHandle(fh);

    MapViewOfFile3(section, NULL, whole_region, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_EXECUTE_READ, NULL, 0);
    MapViewOfFile3(section, NULL, whole_region + n_bytes, 0, n_bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    CloseHandle(section);

    *read_exec = whole_region;
    *read_write = whole_region + n_bytes;
}


void free_jit_buffers(char *read_write, const char *read_exec, size_t n_bytes){
    VirtualFree(read_write, 0, MEM_RELEASE);
    VirtualFree(read_exec, 0, MEM_RELEASE);
}


#endif  // For _WIN32
