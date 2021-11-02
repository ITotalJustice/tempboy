#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ifile/ifile.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


IFile_t* romloader_open(const char* path);
IFile_t* romloader_open_mem(const char* path, const void* data, size_t size);

#ifdef __cplusplus
}
#endif
