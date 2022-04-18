#include <gpt.h>

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
         "USAGE: %s -o <path>\r\n"
         "  -o, --output: Write the output disk image file to this filepath\r\n"
         "\r\n"
         , GPT_IMAGE_VERSION_MAJOR
         , GPT_IMAGE_VERSION_MINOR
         , GPT_IMAGE_VERSION_PATCH
         , GPT_IMAGE_VERSION_TWEAK
         , GPT_IMAGE_ARGV_0
         );
}

int main(int argc, char **argv) {
  GPT_IMAGE_ARGV_0 = argv[0];

  const unsigned sectorSize = 512;
  const char* path = NULL;

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

  initialize_gpt_header(header);
  strcpy((char*)&header->Signature[0], "gptimage");

  FILE* image = fopen(path, "wb+");
  fseek(image, sectorSize, SEEK_SET);
  fwrite(header, sizeof(GPT_HEADER), 1, image);
  fclose(image);

  /* THE IDEA:
   * |-- Open a file for writing (at some path, probably given)
   * |   This will be the disk image file that with GPT partitions.
   * |-- 
   */

  return 0;
}
