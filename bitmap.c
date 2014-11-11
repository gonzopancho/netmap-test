#include "bitmap.h"

extern uint32_t* bitmap_new(size_t n_bits);
extern int bitmap_get(uint32_t *bitmap, size_t n);
extern void bitmap_set(uint32_t *bitmap, size_t n);
extern void bitmap_clear(uint32_t *bitmap, size_t n);
extern void bitmap_setall(uint32_t *bitmap, size_t n_bits);
extern void bitmap_clearall(uint32_t *bitmap, size_t n_bits);
extern void bitmap_clearmask(uint32_t *bitmap, size_t map_bits,
                             uint32_t *mask, size_t mask_bits);
static void char2bin(unsigned char c);

void bitmap_print(uint32_t *bitmap, size_t n_bits) {
  size_t blocks, bytesperblock;
  ssize_t i;
  unsigned char *buf;

  blocks = BITMAP_BLOCKS(n_bits);
  bytesperblock = BITMAP_BLOCKSIZE / CHAR_BIT;
  buf = (unsigned char *)bitmap;

  for (i=blocks-1; i>=0; i--) {
    char2bin(buf[bytesperblock*i+3]);
    printf(" ");
    char2bin(buf[bytesperblock*i+2]);
    printf(" ");
    char2bin(buf[bytesperblock*i+1]);
    printf(" ");
    char2bin(buf[bytesperblock*i]);
    printf(" ");
  }
  printf("\n");
}

static void char2bin(unsigned char c) {
  int i;
  for (i=CHAR_BIT-1; i>=0; i--)
    printf("%d", ((c & (1<<i))>>i));
}
