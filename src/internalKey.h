
#ifndef __LATTE_DISKDB_INTERNALKEY_H
#define __LATTE_DISKDB_INTERNALKEY_H
#include "sds/sds.h"
#include "sequenceNumber.h"
#include <stdbool.h>
#include <stdlib.h>
typedef enum ValueType { 
    kTypeDeletion = 0x0, 
    kTypeValue = 0x1 
} ValueType;

typedef struct InternalKey {
    SequenceNumber seq;
    sds rep;
} InternalKey;

typedef struct ParsedInternalKey {
    sds user_key;
    SequenceNumber sequence;
    ValueType type;
} ParsedInternalKey;
InternalKey* internalKeyCreate(sds user_key, SequenceNumber seq,ValueType type);
InternalKey* internalKeyCopy(InternalKey* internalKey);
void internalKeyDestroy(InternalKey* key);
sds encodeInternalKey(InternalKey* key);
sds internalKey2DebugSds(InternalKey* key);
bool ParseInternalKey(sds internal_key, ParsedInternalKey* result);
// InternalKey* decodeInternalKey(sds value);
// int compareInternalKey(sds a, sds b);

#endif