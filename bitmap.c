#include "bitmap.h"

extern uint32_t* bitmap_new(size_t n_bits);
extern int bitmap_get(uint32_t *bitmap, size_t n);
extern void bitmap_set(uint32_t *bitmap, size_t n);
extern void bitmap_clear(uint32_t *bitmap, size_t n);
extern void bitmap_setall(uint32_t *bitmap, size_t n_bits);
extern void bitmap_clearall(uint32_t *bitmap, size_t n_bits);
extern void bitmap_clearmask(uint32_t *bitmap, size_t map_bits,
                             uint32_t *mask, size_t mask_bits);
