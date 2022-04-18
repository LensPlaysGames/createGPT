#include <gpt.h>

#include <string.h>

void initialize_gpt_header(GPT_HEADER* header) {
  if (!header)
    return;

  memset(&header->Signature[0], 0, 8);
  header->Revision = 0;
  header->Size = 0;
  header->CRC32 = 0;
  header->Reserved0 = 0;
  header->CurrentLBA = 0;
  header->BackupLBA = 0;
  header->FirstUsableLBA = 0;
  header->LastUsableLBA = 0;
  header->DiskGU = 0;
  header->PartitionsTableLBA = 0;
  header->NumberOfPartitionsTableEntries = 0;
  header->PartitionsTableEntrySize = 0;
}

void initialize_gpt_part_entry(GPT_PARTITION_ENTRY* part_entry) {
  if (!part_entry)
    return;

  part_entry->TypeGUID.Data1 = 0;
  part_entry->TypeGUID.Data2 = 0;
  part_entry->TypeGUID.Data3 = 0;
  memset(&part_entry->TypeGUID.Data4[0], 0, 8);
  part_entry->UniqueGUID.Data1 = 0;
  part_entry->UniqueGUID.Data2 = 0;
  part_entry->UniqueGUID.Data3 = 0;
  memset(&part_entry->UniqueGUID.Data4[0], 0, 8);
  memset(&part_entry->Name[0], 0, 72);
  part_entry->Attributes = 0;
  part_entry->StartLBA = 0;
  part_entry->EndLBA = 0;
}
