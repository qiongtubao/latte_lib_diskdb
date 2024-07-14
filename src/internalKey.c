#include "internalKey.h"
#include "sds/sds_plugins.h"
#include <assert.h>
static const ValueType kValueTypeForSeek = kTypeValue;

uint64_t packSequenceAndType(uint64_t seq, ValueType t) {
    assert(seq <= kMaxSequenceNumber);
    assert(t <= kValueTypeForSeek);
    return (seq << 8) | t;
}
sds internalKeytoString(sds user_key, SequenceNumber seq, ValueType type) {
    sds result = sdsnewlen(user_key, sdslen(user_key));
    result =  sdsAppendFixed64(result, packSequenceAndType(seq, type));
    return result;
}

InternalKey* internalKeyCreate(sds user_key, SequenceNumber seq, ValueType type) {
    InternalKey* internalKey = zmalloc(sizeof(InternalKey));
    internalKey->rep = internalKeytoString(user_key, seq, type);
    return internalKey;
}

InternalKey* internalKeyCopy(InternalKey* internalKey) {
    InternalKey* key = zmalloc(sizeof(InternalKey));
    key->rep = sdsdup(internalKey->rep);
    key->seq = internalKey->seq;
    return;
}

sds encodeInternalKey(InternalKey* key) {
    return key->rep;
}
