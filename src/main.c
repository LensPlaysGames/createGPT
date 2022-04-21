#include <crc.h>
#include <guid.h>
#include <gpt.h>
#include <linked_list.h>
#include <mbr.h>

/* TODO:
 * `-- Accept partition name as argument (string conversion nightmare).
 */

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPT_IMAGE_VERSION_MAJOR 0
#define GPT_IMAGE_VERSION_MINOR 0
#define GPT_IMAGE_VERSION_PATCH 1
#define GPT_IMAGE_VERSION_TWEAK 1

char *GPT_IMAGE_ARGV_0 = NULL;

void print_help() {
  printf("createGPT V%u.%u.%u.%u Copyright (C) 2022 Rylan Lens Kellogg\r\n"
         "  Create disk image files with valid GUID Partition Tables.\r\n"
         "\r\n"
         "USAGE: %s -o <path> [-p <path> [--type <guid|preset>]]\r\n"
         "  -o, --output: Write the output disk image file to this filepath\r\n"
         "  -p, --part:   Create a partition from the image at path\r\n"
         "    --type:       Specify a type GUID, or a type preset.\r\n"
         "                  GUID format: 00000000-0000-0000-0000-000000000000\r\n"
         "                  Type presets:\r\n"
         "                    null    -- Zero\r\n"
         "                    system  -- EFI System partition\r\n"
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
  if (msg != NULL) {
    printf("%s\r\n"
           "\r\n"
           , msg
           );
  }
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
              string_to_guid("c12a7328-f81f-11d2-ba4b-00a0c93ec93b"
                             , &partitionContext->GPTEntry.TypeGUID);
            }
            else if (!strcmp(argv[i], "null")) {
              memset(&partitionContext->GPTEntry.TypeGUID, 0, sizeof(GUID));
            }
            else {
              GUID temp;
              if (string_to_guid(argv[i], &temp)) {
                partitionContext->GPTEntry.TypeGUID = temp;
              }
              else {
                print_help_with("Did not recognize partition type");
                return 1;
              }
            }
          }
          else i--;
        }
        partitionContexts = linked_list_add(partitionContext, partitionContexts);
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
  string_to_guid("12345678-6969-0420-b00b-deadbeefcafe", &header.DiskGUID);
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

  free(table);

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

  linked_list_delete_all(partitionContexts, true);

  fseek(image, backupHeader.PartitionsTableLBA * sectorSize, SEEK_SET);
  fwrite(backupTable, 1, sizeof(GPT_PARTITION_TABLE), image);

  free(backupTable);

  memset(sector, 0, sectorSize);
  memcpy(sector, &backupHeader, sizeof(GPT_HEADER));
  fseek(image, header.BackupLBA * sectorSize, SEEK_SET);
  fwrite(sector, 1, sectorSize, image);

  free(sector);

  fclose(image);
  printf("Output image at %s\r\n"
         "\r\n"
         , path
         );

  return 0;
}
