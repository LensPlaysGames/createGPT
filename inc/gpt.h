#ifndef GPT_IMAGE_GPT_H
#define GPT_IMAGE_GPT_H

#include <stdint.h>

typedef struct GUID_T {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} __attribute__((packed)) GUID;

typedef struct GPT_HEADER_T {
  unsigned char Signature[8];
  uint32_t Revision;
  uint32_t Size;
  uint32_t CRC32;
  uint32_t Reserved0;
  uint64_t CurrentLBA;
  uint64_t BackupLBA;
  uint64_t FirstUsableLBA;
  uint64_t LastUsableLBA;
  GUID  DiskGUID;
  uint64_t PartitionsTableLBA;
  uint32_t NumberOfPartitionsTableEntries;
  uint32_t PartitionsTableEntrySize;
  uint32_t PartitionEntryArrayCRC32;
}  __attribute__((packed)) GPT_HEADER;

typedef struct GPT_PARTITION_ENTRY_T {
  GUID TypeGUID;
  GUID UniqueGUID;
  uint64_t StartLBA;
  uint64_t EndLBA;
  uint64_t Attributes;
  // FIXME: Should be 36 UTF-16 code points.
  uint8_t Name[72];
} __attribute__((packed)) GPT_PARTITION_ENTRY;

typedef struct GPT_PARTITION_TABLE_T {
  GPT_PARTITION_ENTRY PartitionEntries[128];
} __attribute__((packed)) GPT_PARTITION_TABLE;

#endif /* GPT_IMAGE_GPT_H */
