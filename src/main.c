#include <stdint.h>
#define _CRT_SECURE_NO_WARNINGS

#include <gpt.h>
#include <mbr.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPT_IMAGE_VERSION_MAJOR 0
#define GPT_IMAGE_VERSION_MINOR 0
#define GPT_IMAGE_VERSION_PATCH 1
#define GPT_IMAGE_VERSION_TWEAK 0

char *GPT_IMAGE_ARGV_0 = NULL;

void print_help() {
  printf("gpt-image V%u.%u.%u.%u Copyright (C) 2022 Rylan Lens Kellogg\r\n"
         "  This is a tool for creating a disk image file with a GUID Partition Table.\r\n"
         "\r\n"
         "USAGE: %s -o <path> [-p <path>]\r\n"
         "  -o, --output:    Write the output disk image file to this filepath\r\n"
         "  -p, --partition: Create a partition from the image at path\r\n"
         "\r\n"
         , GPT_IMAGE_VERSION_MAJOR
         , GPT_IMAGE_VERSION_MINOR
         , GPT_IMAGE_VERSION_PATCH
         , GPT_IMAGE_VERSION_TWEAK
         , GPT_IMAGE_ARGV_0
         );
}

int rc_crc32(uint32_t crc, void *buffer, size_t length) {
  static uint32_t table[256];
  static int have_table = 0;
  uint32_t remainder;
  uint8_t octet;
  int i;
  int j;
  const uint8_t *p;
  const uint8_t *q;
  if (have_table == 0) {
    for (i = 0; i < 256; ++i) {
      remainder = i;
      for (j = 0; j < 8; ++j) {
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
  q = buffer + length;
  for (p = buffer; p < q; ++p) {
    octet = *p;
    crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
  }
  return ~crc;
}

int main(int argc, char **argv) {
  GPT_IMAGE_ARGV_0 = argv[0];

  const unsigned sectorSize = 512;
  const char* path = NULL;

  // TODO: Create singly linked list of partition request structures, or something.
  for (int i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    if (!strcmp(arg, "-o") || !strcmp(arg, "--output")) {
        i++;
        if (argc - i <= 0){
          print_help();
          printf("Expected a filepath following `-o`, `--output`\r\n"
                 "\r\n");
          return 1;
        }
        path = argv[i];
    }
  }

  if (!path) {
    print_help();
    printf("Output filepath is null!\r\n"
           "\r\n");
    return 1;
  }
  
  GPT_PARTITION_TABLE* table = malloc(sizeof(GPT_PARTITION_TABLE));
if (!table) {
    printf("Could not allocate memory for table.\r\n"
           "\r\n");
    return 1;
  }
  memset(table, 0, sectorSize);

  uint8_t *sector = malloc(sectorSize);
  if (!sector) {
    printf("Could not allocate memory for sector.\r\n"
           "\r\n");
    return 1;
  }

  GPT_HEADER *header = malloc(sizeof(GPT_HEADER));
  if (!header) {
    printf("Could not allocate memory for GPT header.\r\n"
           "\r\n");
    return 1;
  }
  memset(header, 0, sizeof(GPT_HEADER));
  strcpy((char*)&header->Signature[0], "EFI PART");
  header->Revision = 1 << 16;
  header->Size = sizeof(GPT_HEADER);
  header->CurrentLBA = 1;
  header->FirstUsableLBA = 34;
  header->LastUsableLBA = 40;
  header->DiskGUID.Data1 = 0;
  header->DiskGUID.Data2 = 0;
  header->DiskGUID.Data3 = 0;
  header->DiskGUID.Data4[0] = 0;
  header->DiskGUID.Data4[1] = 0;
  header->DiskGUID.Data4[2] = 0;
  header->DiskGUID.Data4[3] = 0;
  header->DiskGUID.Data4[4] = 0;
  header->DiskGUID.Data4[5] = 0;
  header->DiskGUID.Data4[6] = 0;
  header->DiskGUID.Data4[7] = 0;
  header->PartitionsTableLBA = 1;
  header->PartitionsTableEntrySize = sizeof(GPT_PARTITION_ENTRY);
  header->CRC32 = rc_crc32(0, header, 0x5c);
  
  GPT_PARTITION_ENTRY *part_entry = malloc(sizeof(GPT_PARTITION_ENTRY));
  if (!part_entry) {
    printf("Could not allocate memory for GPT partition entry.\r\n"
           "\r\n");
    return 1;
  }

  FILE* image = fopen(path, "wb+");
  if (!image) {
    printf("Could not open file at %s\r\n"
           "\r\n"
           , path);
    return 1;
  }

  MASTER_BOOT_RECORD *mbr = malloc(sizeof(MASTER_BOOT_RECORD));
  if (!mbr) {
    printf("Could not allocate memory for MBR\r\n"
           "\r\n");
    return 1;
  }
  memset(mbr, 0, sizeof(MASTER_BOOT_RECORD));

  // Protective MBR
  mbr->Partitions[0].StartSectorCHS[1] = 0x02;
  mbr->Partitions[0].StartLBA = 1;
  // Probably shouldn't do this.
  mbr->Partitions[0].SectorCount = UINT32_MAX;
  // 0xee == GPT Protective MBR Partition Type
  mbr->Partitions[0].PartitionType = 0xee;
  mbr->Partitions[0].LastSectorCHS[0] = 0xff;
  mbr->Partitions[0].LastSectorCHS[1] = 0xff;
  mbr->Partitions[0].LastSectorCHS[2] = 0xff;
  mbr->Magic[0] = 0x55;
  mbr->Magic[1] = 0xaa;
  memset(sector, 0, sectorSize);
  fseek(image, 0, SEEK_SET);
  fwrite(mbr, 512, 1, image);

  // Write GPT Header into sector.
  memset(sector, 0, sectorSize);
  memcpy(sector, header, sizeof(GPT_HEADER));
  // Write sector into LBA1 of disk image.
  fseek(image, sectorSize, SEEK_SET);
  fwrite(sector, sectorSize, 1, image);
  // "allocate" space in file for partition entry sectors.
  fseek(image, sectorSize * 33, SEEK_SET);
  memset(sector, 0, sectorSize);
  memcpy(sector, part_entry, sizeof(GPT_PARTITION_ENTRY));
  fwrite(sector, sectorSize, 1, image);
  // "allocate" space in file for partitions.
  fseek(image, sectorSize * 42, SEEK_CUR);
  fwrite("0", 1, 1, image);
  // "allocate" space in file for backup header and partition entry sectors.
  memset(sector, 0, sectorSize);
  memcpy(sector, part_entry, sizeof(GPT_PARTITION_ENTRY));
  fwrite(sector, sectorSize, 1, image);

  fseek(image, sectorSize * 32, SEEK_CUR);

  memset(sector, 0, sectorSize);
  memcpy(sector, header, sizeof(GPT_HEADER));
  fwrite(sector, sectorSize, 1, image);

  fclose(image);

  /* TODO:
   * For every partition, create an entry in the GPT, as well as the space in the file.
   */

  return 0;
}
