#ifndef MEMCPY_BWA_H
#define MEMCPY_BWA_H

#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "safe_mem_lib.h"
#ifdef __cplusplus
}
#endif

errno_t memcpy_bwamem(void *dest, rsize_t dmax, const void *src, rsize_t smax, char *file_name, int line_num);

#endif
