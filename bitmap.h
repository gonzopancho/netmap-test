#ifndef _BITMAP_
#define _BITMAP_

#include <stdint.h>   // uint32_t
#include <stddef.h>   // size_t
#include <stdlib.h>   // calloc
#include <limits.h>   // CHAR_BIT

#define BITMAP_BLOCKSIZE 32 // using uint32_t, which guarantees 32 bits

#if BITMAP_BLOCKSIZE == 32
  #define BITMAP_BIT(n) (1U << ((n) & 31))
  #define BITMAP_BLOCKS(n) (((n) + BITMAP_BLOCKSIZE - 1) >> 5)
  #define BITMAP_IDX(n) ((n) >> 5)
#else
  #define BITMAP_BIT(n) (1U << ((n) % BITMAP_BLOCKSIZE))
  #define BITMAP_BLOCKS(n) (((n) + BITMAP_BLOCKSIZE - 1) / BITMAP_BLOCKSIZE)
  #define BITMAP_IDX(n) ((n) / BITMAP_BLOCKSIZE)
#endif

inline uint32_t* bitmap_new(size_t n_bits) {
  size_t blocks;

  if (!n_bits)
    return NULL;

  blocks = BITMAP_BLOCKS(n_bits);
  return calloc(1, blocks * sizeof(uint32_t));
}

inline int bitmap_get(uint32_t *bitmap, size_t n) {
  return bitmap[BITMAP_IDX(n)] & BITMAP_BIT(n);
}

inline void bitmap_set(uint32_t *bitmap, size_t n) {
  bitmap[BITMAP_IDX(n)] |= BITMAP_BIT(n);
}

inline void bitmap_clear(uint32_t *bitmap, size_t n) {
  bitmap[BITMAP_IDX(n)] &= ~BITMAP_BIT(n);
}

inline void bitmap_setall(uint32_t *bitmap, size_t n_bits) {
  size_t blocks, i;

  blocks = BITMAP_BLOCKS(n_bits);
  for (i=0; i < blocks; i++)
    bitmap[i] = UINT32_MAX;
}

inline void bitmap_clearall(uint32_t *bitmap, size_t n_bits) {
  size_t blocks, i;

  blocks = BITMAP_BLOCKS(n_bits);
  for (i=0; i < blocks; i++)
    bitmap[i] = 0;
}

inline void bitmap_clearmask(uint32_t *bitmap, size_t map_bits,
                    uint32_t *mask, size_t mask_bits) {
  size_t limit, i;
  limit = mask_bits < map_bits ? mask_bits : map_bits;
  limit = BITMAP_BLOCKS(limit);

  for (i=0; i < limit; i++)
    bitmap[i] &= ~mask[i];
}

#endif // _BITMAP_
