#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>    // printf
#include <netinet/in.h>   // htonl
#include "bitmap.h"

int main() {
  size_t len, i;
  uint32_t *bitmap;
  uint32_t *mask;

  len = 256;
  bitmap = bitmap_new(len);
  assert(bitmap);

  mask = bitmap_new(len);
  assert(mask);

  for (i=0; i < len; i++)
    assert(bitmap_get(bitmap, i) == 0);

  for (i=0; i < len; i++)
    bitmap_set(bitmap, i);

  for (i=0; i*32 < len; i++)
    assert(bitmap[i] == UINT32_MAX);

  for (i=0; i < len; i++)
    bitmap_clear(bitmap, i);

  for (i=0; i*32 < len; i++)
    assert(bitmap[i] == 0);

  bitmap_setall(bitmap, len);
  for (i=0; i*32 < len; i++)
    assert(bitmap[i] == UINT32_MAX);

  bitmap_clearall(bitmap, len);
  for (i=0; i*32 < len; i++)
    assert(bitmap[i] == 0);

  // setup dummy data for clearmask test
  for (i=0; i < len; i++) {
    if ((i & 31) == 31)
      continue;
    bitmap_set(bitmap, i);
  }
  assert(bitmap[0] == (UINT32_MAX >> 1));

  for (i=0; i < len; i+=32)
    bitmap_set(mask, i);

  bitmap_clearmask(bitmap, len, mask, len);
  for (i=0; i*32 < len; i++) {
    assert(mask[i] == 1); // check that mask is untouched
    assert(bitmap[i] == (UINT32_MAX >> 1) - 1); // check that bits are cleared
  }

  exit(EXIT_SUCCESS);
}
