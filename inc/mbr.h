#ifndef GPT_IMAGE_MBR_H
#define GPT_IMAGE_MBR_H

#include <stdint.h>

typedef struct MASTER_BOOT_RECORD_ENTRY_T {
  uint8_t Status;
  uint8_t StartSectorCHS[3];
  uint8_t PartitionType;
  uint8_t LastSectorCHS[3];
  uint32_t StartLBA;
  uint32_t SectorCount;
} __attribute__((packed)) MASTER_BOOT_RECORD_ENTRY;

typedef struct MASTER_BOOT_RECORD_T {
  uint8_t BootstrapCode[446];
  MASTER_BOOT_RECORD_ENTRY Partitions[4];
  uint8_t Magic[2];
} __attribute__((packed)) MASTER_BOOT_RECORD;

#endif /* GPT_IMAGE_MBR_H */
