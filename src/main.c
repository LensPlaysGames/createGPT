#include <gpt.h>
#include <mbr.h>

#define _CRT_SECURE_NO_WARNINGS
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
         "  This is a tool for creating a disk image file with a GUID Partition Table.\r\n"
         "\r\n"
         "USAGE: %s -o <path> [-p <path>]\r\n"
         "  -o, --output: Write the output disk image file to this filepath\r\n"
         "  -p, --part:   Create a partition from the image at path\r\n"
         "\r\n"
         , GPT_IMAGE_VERSION_MAJOR
         , GPT_IMAGE_VERSION_MINOR
         , GPT_IMAGE_VERSION_PATCH
         , GPT_IMAGE_VERSION_TWEAK
         , GPT_IMAGE_ARGV_0
         );
}

// TODO: Fill partition table CRC32 field in header and
//       backup header with proper checksum value.
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

typedef struct PARTITION_REQUEST_T {
  const char* Path;
  FILE* File;  // Input file file descriptor
  size_t Size; // Size in bytes of input file
  GPT_PARTITION_ENTRY Partition;
} PARTITION_REQUEST;

int main(int argc, char **argv) {
  GPT_IMAGE_ARGV_0 = argv[0];

  const unsigned sectorSize = 512;
  const char* path = NULL;

  LINKED_LIST *partitionImagePaths = NULL;
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
    if (!strcmp(arg, "-p")
        || !strcmp(arg, "--part")
        || !strcmp(arg, "--partition"))
      {
      i++;
      if (argc - i <= 0) {
        print_help();
        printf("Expected a filepath following `-p`, `--partition`\r\n"
               "\r\n");
        return 1;
      }
      if (partitionImagePaths == NULL) {
        partitionImagePaths = malloc(sizeof(LINKED_LIST));
        partitionImagePaths->Data = &argv[i];
        partitionImagePaths->Next = NULL;
      }
      else partitionImagePaths = linked_list_add(&argv[i], partitionImagePaths);
    }
  }

  if (!path) {
    print_help();
    printf("Output filepath is null!\r\n"
           "\r\n");
    return 1;
  }

  size_t BeginDataLBA = 34;

  LINKED_LIST *partPath = partitionImagePaths;
  LINKED_LIST *partRequests = NULL;
  while (partPath != NULL) {
    FILE* fp = fopen(*(char**)partPath->Data, "r");
    if (!fp) {
      printf("Could not open %s for reading.\r\n"
             "\r\n"
             , *(char**)partPath->Data);
      return 1;
    }
    else {
      PARTITION_REQUEST *partRequest = malloc(sizeof(PARTITION_REQUEST));
      if (!partRequest) {
        printf("Could not allocate memory for partition request\r\n"
               "\r\n");
        return 1;
      }
      partRequest->Path = *(char**)partPath->Data;
      partRequest->File = fp;
      fseek(fp, 0, SEEK_END);
      partRequest->Size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      GPT_PARTITION_ENTRY partEntry;
      partEntry.TypeGUID.Data1 = 0xc12a7328;
      partEntry.TypeGUID.Data2 = 0xf81f;
      partEntry.TypeGUID.Data3 = 0x11d2;
      partEntry.TypeGUID.Data4[0] = 0xba;
      partEntry.TypeGUID.Data4[1] = 0x4b;
      partEntry.TypeGUID.Data4[2] = 0x00;
      partEntry.TypeGUID.Data4[3] = 0xa0;
      partEntry.TypeGUID.Data4[4] = 0xc9;
      partEntry.TypeGUID.Data4[5] = 0x3e;
      partEntry.TypeGUID.Data4[6] = 0xc9;
      partEntry.TypeGUID.Data4[7] = 0x3b;
      partEntry.StartLBA = BeginDataLBA;
      size_t partSectors = (partRequest->Size / sectorSize) + 1;
      BeginDataLBA += partSectors + 1;
      partEntry.EndLBA = partEntry.StartLBA + partSectors;
      partEntry.Attributes = 0;
      partRequest->Partition = partEntry;
      if (!partRequests) {
        partRequests = malloc(sizeof(LINKED_LIST));
        partRequests->Data = partRequest;
        partRequests->Next = NULL;
      }
      else partRequests = linked_list_add(partRequest, partRequests);
    }
    partPath = partPath->Next;
  }

  // TODO: Clean up partitionImagePaths list.

  LINKED_LIST *partRequest = partRequests;
  while (partRequest != NULL) {
    PARTITION_REQUEST *request = (PARTITION_REQUEST*)partRequest->Data;
    printf("Partition Request:\r\n"
           "  Path: %s\r\n"
           "  Size: %zu\r\n"
           "  StartLBA: %zu\r\n"
           "  EndLBA: %zu\r\n"
           , request->Path
           , request->Size
           , request->Partition.StartLBA
           , request->Partition.EndLBA
           );
    partRequest = partRequest->Next;
  }

  GPT_HEADER header;
  memset(&header, 0, sizeof(GPT_HEADER));
  // Header information
  memcpy(&header.Signature[0], "EFI PART", 8);
  header.Revision = 1 << 16;
  header.Size = sizeof(GPT_HEADER);
  // LBAs
  header.CurrentLBA = 1;
  header.FirstUsableLBA = 34;
  header.LastUsableLBA = BeginDataLBA - 1;
  header.BackupLBA = header.LastUsableLBA + 33;
  // GUID
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
  header.CRC32 = rc_crc32(0, &header, header.Size);

  // TODO: Build partition entry array before GPT header.
  //       Calculate CRC32 of part. entry array and store in GPT header.
  //header.PartitionEntryArrayCRC32 = rc_crc32(0, address, byteCount);

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

  FILE* image = fopen(path, "wb+");
  if (!image) {
    printf("Could not open file at %s\r\n"
           "\r\n"
           , path);
    return 1;
  }

  // Write protective MBR to LBA0.
  fseek(image, 0, SEEK_SET);
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

  // Write partition table to memory and disk.
  fseek(image, sectorSize * 2, SEEK_SET);
  GPT_PARTITION_TABLE* partTable = malloc(sizeof(GPT_PARTITION_TABLE));
  if (!partTable) {
    printf("Could not allocate memory for partition entry table.\r\n"
           "\r\n");
    return 1;
  }
  memset(partTable, 0, sizeof(GPT_PARTITION_TABLE));
  GPT_PARTITION_ENTRY* partEntry = (GPT_PARTITION_ENTRY*)partTable;
  LINKED_LIST *part = partRequests;
  for (int i = 0; i < 128; ++i) {
    if (part) {
      memcpy(partEntry
             , &((PARTITION_REQUEST*)part->Data)->Partition
             , sizeof(GPT_PARTITION_ENTRY));
      part = part->Next;
    }
    fwrite(partEntry, 1, sizeof(GPT_PARTITION_ENTRY), image);
    partEntry++;
  }

  size_t tableCRC = rc_crc32(0, partTable, sizeof(GPT_PARTITION_TABLE));
  header.PartitionEntryArrayCRC32 = tableCRC;

  //memset(sector, 0, sectorSize);
  //memcpy(sector, &header, sizeof(GPT_HEADER));
  //fseek(image, sectorSize, SEEK_SET);
  //fwrite(sector, 1, sectorSize, image);

  //fseek(image, sectorSize * 2, SEEK_SET);
  //memset(sector, 0, sectorSize);
  LINKED_LIST *partitions = partRequests;
  //for (int i = 0; i < 128; ++i) {
  //  memset(sector, 0, sectorSize);
  //  if (partitions) {
  //    fwrite(&((PARTITION_REQUEST*)partitions->Data)->Partition
  //           , 1
  //           , sizeof(GPT_PARTITION_ENTRY)
  //           , image);
  //    partitions = partitions->Next;
  //  }
  //  else fwrite(sector, 1, sizeof(GPT_PARTITION_ENTRY), image);
  //}


  // Write partitions
  fseek(image, header.FirstUsableLBA * sectorSize, SEEK_SET);
  partitions = partRequests;
  while (partitions != NULL) {
    memset(sector, 0, sectorSize);
    PARTITION_REQUEST* request = (PARTITION_REQUEST*)partitions->Data;
    const size_t bufferSize = 1024 << 10;
    void* buffer = malloc(bufferSize);
    if (!buffer) {
      printf("Could not allocate memory for partition buffer\r\n"
             "\r\n");
      return 1;
    }
    size_t bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, bufferSize, request->File) > 0)) {
      fwrite(buffer, 1, bytesRead, image);
    }
    free(buffer);
    partitions = partitions->Next;
  }

  // Write backup partition table.
  memset(sector, 0, sectorSize);
  partitions = partRequests;
  for (int i = 0; i < 128; ++i) {
    if (partitions) {
      fwrite(&((PARTITION_REQUEST*)partitions->Data)->Partition
             , 1
             , sizeof(GPT_PARTITION_ENTRY)
             , image);
      partitions = partitions->Next;
    }
    else fwrite(sector, 1, sizeof(GPT_PARTITION_ENTRY), image);
  }

  // Prepare backup GPT header.
  GPT_HEADER backup = header;
  backup.PartitionsTableLBA = header.BackupLBA - 32;
  backup.CurrentLBA = header.BackupLBA;
  backup.BackupLBA = header.CurrentLBA;
  // Write backup GPT header.
  memset(sector, 0, sectorSize);
  memcpy(sector, &backup, sizeof(GPT_HEADER));
  fseek(image, sectorSize * header.BackupLBA, SEEK_SET);
  fwrite(sector, 1, sectorSize, image);

  // Uncommenting this anywhere breaks the output.
  // This doesn't make sense as it should just be copying
  // the same input to the same output no matter where or when it's called.
  // What am I doing wrong??
  //memset(sector, 0, sectorSize);
  //memcpy(sector, &header, sizeof(GPT_HEADER));
  //rewind(image);
  //fseek(image, sectorSize, SEEK_SET);
  //fwrite(&header, 1, sizeof(GPT_HEADER), image);

  fclose(image);
  free(sector);

  return 0;
}
