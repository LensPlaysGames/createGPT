#ifndef CRC_H
#define CRC_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32(uint32_t crc, void *buffer, size_t length) {
  static uint32_t table[256];
  static int have_table = 0;
  if (have_table == 0) {
    for (int i = 0; i < 256; ++i) {
      uint32_t remainder = i;
      for (uint8_t j = 0; j < 8; ++j) {
        if (remainder & 1) {
          remainder >>= 1;
          remainder ^= 0xedb88320;
        }
        else remainder >>= 1;
      }
      table[i] = remainder;
    }
    have_table = 1;
  }
  crc = ~crc;
  const uint8_t *q = (uint8_t *)((uintptr_t)buffer + length);
  for (const uint8_t *p = buffer; p < q; ++p) {
    uint8_t octet = *p;
    crc = (crc >> 8) ^ table[(crc ^ octet) & 0xff];
  }
  return ~crc;
}

#endif /* CRC_H */
