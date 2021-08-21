#include <memory.h>
#include <stdlib.h>
void *
__memcpy_chk (void *dest, const void *src, size_t len, size_t dstlen)
{
  return memcpy (dest, src, len);
}

void *
__memset_chk (void *dest, int val, size_t len, size_t dstlen)
{
  return memset (dest, val, len);
}

void *
__memmove_chk (void *dest, const void *src, size_t len, size_t dstlen)
{
  return memmove (dest, src, len);
}