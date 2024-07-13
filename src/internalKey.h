
#ifndef __LATTE_DISKDB_INTERNALKEY_H
#define __LATTE_DISKDB_INTERNALKEY_H
#include "sds/sds.h"
#include "sequenceNumber.h"
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
} ParsedInternalKey;
InternalKey* internalKeyCreate(sds user_key, SequenceNumber seq,ValueType type);
sds encodeInternalKey(InternalKey* key);
// InternalKey* decodeInternalKey(sds value);
// int compareInternalKey(sds a, sds b);

#endif