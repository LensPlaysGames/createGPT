#include <guid.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define G32 "%08" SCNx32
#define G16 "%04" SCNx16
#define G8  "%02" SCNx8

bool string_to_guid(const char *str, GUID *guid) {
    if (!str) {
        printf("Can not convert null string into GUID\r\n"
               "\r\n");
        return false;
    }
    if (!guid) {
        printf("Can not convert string into null GUID\r\n"
               "\r\n");
        return false;
    }
    if (strlen(str) != GUID_STRING_LENGTH) {
        printf("GUID string is of incorrect length\r\n"
               "\r\n");
        return false;
    }
  
    if (str[8] != '-'
        || str[13] != '-'
        || str[18] != '-'
        || str[23] != '-')
    {
        printf("Didn't find dashes in GUID string\r\n"
               "\r\n");
        return false;
    }

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
    if (nfields != 11) {
        printf("nfields is not correct (expecting 11): %i\r\n"
               "\r\n"
               , nfields
               );
        return false;
    }
    if (nchars != GUID_STRING_LENGTH) {
        printf("nchars is not correct (expecting %i): %i\r\n"
               "\r\n"
               , (int)GUID_STRING_LENGTH
               , nchars
               );
        return false;
    }
    return true;
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
