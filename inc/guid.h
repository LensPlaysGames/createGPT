#ifndef CREATEGPT_GUID_H
#define CREATEGPT_GUID_H

#include <stdbool.h>
#include <stdint.h>

#define GUID_STRING_LENGTH 36

typedef struct GUID_T {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} GUID;

/* Convert a string in the format of:
 * 00000000-0000-0000-0000-000000000000
 * into a GUID data structure.
 *
 * Return Value: false upon conversion failure, otherwise true.
 */
bool string_to_guid(const char *str, GUID *guid);

void print_guid(GUID *guid);

#endif /* CREATEGPT_GUID_H */
