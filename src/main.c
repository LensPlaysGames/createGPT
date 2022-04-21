#include <gpt.h>
#include <mbr.h>

/* TODO:
 * |-- Clean up memory more better.
 * `-- Accept more partition arguments (name, more lenient GUID, etc).
 */

#define _CRT_SECURE_NO_WARNINGS
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
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
         "  Create disk image files with valid GUID Partition Tables.\r\n"
         "\r\n"
         "USAGE: %s -o <path> [-p <path> [--type <guid|preset>]]\r\n"
         "  -o, --output: Write the output disk image file to this filepath\r\n"
         "  -p, --part:   Create a partition from the image at path\r\n"
         "                Type presets:\r\n"
         "                  system  -- EFI System partition\r\n"
         "\r\n"
         , GPT_IMAGE_VERSION_MAJOR
         , GPT_IMAGE_VERSION_MINOR
         , GPT_IMAGE_VERSION_PATCH
         , GPT_IMAGE_VERSION_TWEAK
         , GPT_IMAGE_ARGV_0
         );
}

void print_help_with(const char* msg) {
  print_help();
  printf("%s\r\n"
         "\r\n"
         , msg
         );
}

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

#define G32 "%08" SCNx32
#define G16 "%04" SCNx16
#define G8  "%02" SCNx8

bool string_to_guid(const char *str, GUID *guid) {
  if (!guid)
    return false;

  int nchars = -1;
  int nfields =
    sscanf(str
           , G32 "-" G16 "-" G16 "-" G8 G8 "-" G8 G8 G8 G8 G8 G8 "%n"
           , &guid->Data1, &guid->Data2, &guid->Data3
           , &guid->Data4[0], &guid->Data4[1]
           , &guid->Data4[2], &guid->Data4[3]
           , &guid->Data4[4], &guid->Data4[5]
           , &guid->Data4[6], &guid->Data4[7]
           , &nchars
           );
  return nfields == 11 && nchars == 38;
}

void print_guid(GUID *guid) {
  if (!guid)
    return;

  printf(G32 "-" G16 "-" G16 "-" G8 G8 "-" G8 G8 G8 G8 G8 G8
         , guid->Data1, guid->Data2, guid->Data3
         , guid->Data4[0], guid->Data4[1]
         , guid->Data4[2], guid->Data4[3]
         , guid->Data4[4], guid->Data4[5]
         , guid->Data4[6], guid->Data4[7]
         );
}

#undef G32
#undef G16
#undef G8

typedef struct LINKED_LIST_T {
  void* Data;
  struct LINKED_LIST_T *Next;
} LINKED_LIST;

LINKED_LIST* linked_list_add(void* newData, LINKED_LIST *list) {
  LINKED_LIST *newNode = malloc(sizeof(LINKED_LIST));
  newNode->Data = newData;
  newNode->Next = list;
  return newNode;
}

typedef struct PARTITION_CONTEXT_T {
  const char* ImagePath;
  FILE* File;
  size_t FileSize;
  GPT_PARTITION_ENTRY GPTEntry;
} PARTITION_CONTEXT;

int main(int argc, char **argv) {
  GPT_IMAGE_ARGV_0 = argv[0];

  const unsigned sectorSize = 512;
  const char* path = NULL;

  size_t dataSectorsOffset = 34;
  LINKED_LIST *partitionContexts = NULL;
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (!strcmp(arg, "-o") || !strcmp(arg, "--output")) {
      i++;
      if (argc - i <= 0){
        print_help_with("Expected a filepath following `-o`, `--output`");
        return 1;
      }
      path = argv[i];
    }
    if (!strcmp(arg, "-p")
        || !strcmp(arg, "--part")
        || !strcmp(arg, "--partition"))
      {
        i++;
        if (argc - i <= 0) {
          print_help_with("Expected a filepath following `-p`, `--partition`");
          return 1;
        }
        static bool partitionContextsWasNull = false;
        if (partitionContexts == NULL) {
          partitionContextsWasNull = true;
          partitionContexts = malloc(sizeof(LINKED_LIST));
          partitionContexts->Next = NULL;
        }
        PARTITION_CONTEXT *partitionContext = malloc(sizeof(PARTITION_CONTEXT));
        if (!partitionContext) {
          printf("Couldn't allocate memory for partition context\r\n");
          return 1;
        }
        memset(partitionContext, 0, sizeof(PARTITION_CONTEXT));
        partitionContext->ImagePath = argv[i];
        FILE *image = fopen(partitionContext->ImagePath, "rb");
        if (!image) {
          printf("Could not open partition image at %s"
                 , partitionContext->ImagePath);
          return 1;
        }
        fseek(image, 0, SEEK_END);
        partitionContext->FileSize = ftell(image);
        fseek(image, 0, SEEK_SET);
        memset(&partitionContext->GPTEntry.Name[0], '\0', 72);
        partitionContext->GPTEntry.Attributes = 0;
        partitionContext->GPTEntry.StartLBA = (uint64_t)dataSectorsOffset;
        size_t partitionSectorCount = (partitionContext->FileSize / sectorSize) + 1;
        partitionContext->GPTEntry.EndLBA = dataSectorsOffset + partitionSectorCount - 1;
        dataSectorsOffset += partitionSectorCount;
        partitionContext->File = image;
        if (argc - i >= 2) {
          i++;
          if (!strcmp(argv[i], "--type")) {
            i++;
            if (!strcmp(argv[i], "system")) {
              partitionContext->GPTEntry.TypeGUID.Data1 = 0xc12a7328;
              partitionContext->GPTEntry.TypeGUID.Data2 = 0xf81f;
              partitionContext->GPTEntry.TypeGUID.Data3 = 0x11d2;
              partitionContext->GPTEntry.TypeGUID.Data4[0] = 0xba;
              partitionContext->GPTEntry.TypeGUID.Data4[1] = 0x4b;
              partitionContext->GPTEntry.TypeGUID.Data4[2] = 0x00;
              partitionContext->GPTEntry.TypeGUID.Data4[3] = 0xa0;
              partitionContext->GPTEntry.TypeGUID.Data4[4] = 0xc9;
              partitionContext->GPTEntry.TypeGUID.Data4[5] = 0x3e;
              partitionContext->GPTEntry.TypeGUID.Data4[6] = 0xc9;
              partitionContext->GPTEntry.TypeGUID.Data4[6] = 0x3b;
            }
            else {
              i -= 2;
              print_help_with("Did not recognize type");
              return 1;
            }
          }
          else i--;
        }

        if (argc - i <= 1)
            i--;
        else {

        }
        if (partitionContextsWasNull) {
          partitionContexts->Data = partitionContext;
          partitionContextsWasNull = false;
        }
        else partitionContexts = linked_list_add(partitionContext, partitionContexts);
      }
  }

  if (!path) {
    print_help_with("Output filepath is null!");
    return 1;
  }

  GPT_PARTITION_TABLE *table = malloc(sizeof(GPT_PARTITION_TABLE));
  if (!table) {
    printf("Couldn't allocate memory for main partition table\r\n"
           "\r\n");
    return 1;
  }
  memset(table, 0, sizeof(GPT_PARTITION_TABLE));

  GPT_PARTITION_TABLE *backupTable = malloc(sizeof(GPT_PARTITION_TABLE));
  if (!backupTable) {
    printf("Couldn't allocate memory for backup partition table\r\n"
           "\r\n");
    return 1;
  }

  memset(backupTable, 0, sizeof(GPT_PARTITION_TABLE));

  uint64_t partitionsLastLBA = 34;
  LINKED_LIST *partContext = partitionContexts;
  GPT_PARTITION_ENTRY *tableIterator = (GPT_PARTITION_ENTRY *)table;
  while (partContext != NULL) {
    PARTITION_CONTEXT *part = ((PARTITION_CONTEXT *)partContext->Data);
    // FIXME: Rudimentary handling of UCS-2 characters
    memset(part->GPTEntry.Name, 0, 72);
    memcpy(part->GPTEntry.Name, u"EFI System                         ", 36);
    printf("Partition Context:\r\n");
    printf("  Name:     \"");
    char *name = (char*)&part->GPTEntry.Name[0];
    for (unsigned i = 0; i < 72; i += 2) {
      printf("%c", name[i]);
    }
    printf("\"\r\n");
    printf("  Path:     \"%s\"\r\n"
           "  Size:     %zu\r\n"
           "  StartLBA: %"SCNd64"\r\n"
           "  EndLBA:   %"SCNd64"\r\n"
           , part->ImagePath
           , part->FileSize
           , part->GPTEntry.StartLBA
           , part->GPTEntry.EndLBA
           );
    printf("  Type:     ");
    print_guid(&part->GPTEntry.TypeGUID);
    printf("\r\n"
           "  Unique:   ");
    print_guid(&part->GPTEntry.UniqueGUID);
    printf("\r\n\r\n");
    
    uint64_t endLBA = part->GPTEntry.EndLBA;
    partitionsLastLBA = endLBA > partitionsLastLBA ? endLBA : partitionsLastLBA;

    memcpy(tableIterator, &part->GPTEntry, sizeof(GPT_PARTITION_ENTRY));

    partContext = partContext->Next;
    tableIterator++;
  }
  memcpy(backupTable, table, sizeof(GPT_PARTITION_TABLE));

  GPT_HEADER header;
  memset(&header, 0, sizeof(GPT_HEADER));
  // Header information
  // FIXME: Signature passed on cmd line!
  memcpy(&header.Signature[0], "EFI PART", 8);
  header.Revision = 1 << 16;
  header.Size = sizeof(GPT_HEADER);
  // LBAs
  header.CurrentLBA = 1;
  header.FirstUsableLBA = 34;
  header.LastUsableLBA = partitionsLastLBA;
  header.BackupLBA = header.LastUsableLBA + 33;
  // GUID
  // FIXME: Actually generate a random GUID.
  header.DiskGUID.Data1 = 8907;
  header.DiskGUID.Data2 = 897;
  header.DiskGUID.Data3 = 981;
  header.DiskGUID.Data4[0] = 128;
  header.DiskGUID.Data4[1] = 238;
  header.DiskGUID.Data4[2] = 231;
  header.DiskGUID.Data4[3] = 85;
  header.DiskGUID.Data4[4] = 10;
  header.DiskGUID.Data4[5] = 93;
  header.DiskGUID.Data4[6] = 152;
  header.DiskGUID.Data4[7] = 255;
  // Partitions
  header.PartitionsTableLBA = 2;
  header.NumberOfPartitionsTableEntries = 128;
  header.PartitionsTableEntrySize = sizeof(GPT_PARTITION_ENTRY);
  header.PartitionEntryArrayCRC32 = 0;
  header.Reserved0 = 0;
  header.CRC32 = 0;
  header.PartitionEntryArrayCRC32 = crc32(0, table, sizeof(GPT_PARTITION_TABLE));
  header.CRC32 = crc32(0, &header, header.Size);

  GPT_HEADER backupHeader = header;
  backupHeader.CRC32 = 0;
  backupHeader.CurrentLBA = header.BackupLBA;
  backupHeader.BackupLBA = header.CurrentLBA;
  backupHeader.PartitionsTableLBA = header.LastUsableLBA + 1;
  backupHeader.CRC32 = crc32(0, &backupHeader, backupHeader.Size);

  MASTER_BOOT_RECORD protectiveMBR;
  memset(&protectiveMBR, 0, sizeof(MASTER_BOOT_RECORD));
  protectiveMBR.Partitions[0].StartSectorCHS[1] = 0x02;
  protectiveMBR.Partitions[0].StartLBA = 1;
  protectiveMBR.Partitions[0].SectorCount = header.BackupLBA;
  // 0xee == GPT Protective MBR Partition Type
  protectiveMBR.Partitions[0].PartitionType = 0xee;
  protectiveMBR.Partitions[0].LastSectorCHS[0] = 0xff;
  protectiveMBR.Partitions[0].LastSectorCHS[1] = 0xff;
  protectiveMBR.Partitions[0].LastSectorCHS[2] = 0xff;
  protectiveMBR.Magic[0] = 0x55;
  protectiveMBR.Magic[1] = 0xaa;

  FILE* image = fopen(path, "wb");
  if (!image) {
    printf("Could not open file at %s\r\n"
           "\r\n"
           , path);
    return 1;
  }
  // Write protective MBR to LBA0.
  fwrite(&protectiveMBR, 1, 512, image);

  // Write main GPT header.
  uint8_t *sector = malloc(sectorSize);
  if (!sector) {
    printf("Could not allocate memory for sector.\r\n"
           "\r\n");
    return 1;
  }
  memset(sector, 0, sectorSize);
  memcpy(sector, &header, sizeof(GPT_HEADER));
  fseek(image, sectorSize, SEEK_SET);
  fwrite(sector, 1, sectorSize, image);

  // Write main partition table
  fseek(image, header.PartitionsTableLBA * sectorSize, SEEK_SET);
  fwrite(table, 1, sizeof(GPT_PARTITION_TABLE), image);

  // Write contents of partitions.
  partContext = partitionContexts;
  while (partContext != NULL) {
    // Copy partition file to output image file.
    PARTITION_CONTEXT* part = ((PARTITION_CONTEXT *)partContext->Data);
    fseek(image, part->GPTEntry.StartLBA * sectorSize, SEEK_SET);
    uint64_t bytesLeft = part->FileSize;
    const uint64_t copyByAmount = 512;
    uint8_t *buffer = malloc(copyByAmount);
    while (bytesLeft) {
      memset(buffer, 0, copyByAmount);
      size_t read = fread(buffer, 1, copyByAmount, part->File);
      if (bytesLeft > copyByAmount && read < copyByAmount) {
        printf("Error when reading partition image during copying.\r\n"
               "\r\n");
        return 1;
      }
      size_t written = fwrite(buffer, 1, copyByAmount, image);
      if (written != copyByAmount) {
        printf("Error when writing partition to image during copying.\r\n"
               "\r\n");
        return 1;
      }
      bytesLeft -= copyByAmount;
    }
    free(buffer);
    partContext = partContext->Next;
  }

  fseek(image, backupHeader.PartitionsTableLBA * sectorSize, SEEK_SET);
  fwrite(backupTable, 1, sizeof(GPT_PARTITION_TABLE), image);

  memset(sector, 0, sectorSize);
  memcpy(sector, &backupHeader, sizeof(GPT_HEADER));
  fseek(image, header.BackupLBA * sectorSize, SEEK_SET);
  fwrite(sector, 1, sectorSize, image);

  fclose(image);
  printf("Output image at %s\r\n"
         "\r\n"
         , path
         );

  return 0;
}
