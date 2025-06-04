#include "memcpy_bwamem.h"

errno_t memcpy_bwamem(void *dest, rsize_t dmax, const void *src, rsize_t smax, char *file_name, int line_num)
{
    errno_t ret;
    int64_t bytes_copied;
    for(bytes_copied = 0; bytes_copied < smax; bytes_copied += RSIZE_MAX_MEM)
    {
        int64_t bytes_remaining = smax - bytes_copied;
        int64_t bytes_to_copy = (bytes_remaining > RSIZE_MAX_MEM) ? RSIZE_MAX_MEM : bytes_remaining;
        int64_t dest_bytes_remaining = dmax - bytes_copied;
        int64_t dest_bytes = (dest_bytes_remaining < bytes_to_copy) ? dest_bytes_remaining : bytes_to_copy;
        if((ret = memcpy_s((char *)dest + bytes_copied, dest_bytes, (const char *)src + bytes_copied, bytes_to_copy)) != 0)
        {
            fprintf(stderr, "[%s: %d] memcpy_s returned %d\n", file_name, line_num, ret);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
