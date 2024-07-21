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

void internalKeyDestroy(InternalKey* key) {
    sdsfree(key->rep);
}

sds encodeInternalKey(InternalKey* key) {
    return key->rep;
}


//返回user_key字符串
//密钥 ：{user_key}{8位 sequence + type}
sds extractUserKey(sds internal_key) {
    return sdsnewlen(internal_key, sdslen(internal_key) - 8);
}


bool ParseInternalKey(sds internal_key, ParsedInternalKey* result) {
    const size_t n = sdslen(internal_key);
    if (n < 8) return false;
    uint64_t num = decodeFixed64(internal_key + n - 8);
    uint8_t c = num & 0xff;
    result->sequence = num >> 8;
    result->type = (ValueType)(c);
    result->user_key = sdsnewlen(internal_key, n - 8);
    return c <= (uint8_t)kTypeValue;
}

sds escapeSds(sds result, sds key) {
    for (size_t i = 0; i < sdslen(key); i++) {
        char c = key[i];
        if (c >= ' ' && c <= '~') {
            result = sdscat(result, &c);
        } else {
            result = sdscatprintf(result, "\\x%02x", (unsigned int)(c) & 0xff);
        }
    }
    return result;
}
sds ParsedInternalKey2DebugSds(ParsedInternalKey* parsed) {
    sds result = sdsnew('\'');
    result = escapeSds(result, parsed->user_key);
    result = sdscatfmt(result,"%s%lld%d", "' @ ", parsed->sequence, parsed->type);
    return result;
}
sds internalKey2DebugSds(InternalKey* key) {
    ParsedInternalKey parsed;
    if (ParseInternalKey(key->rep, &parsed)) {
        return ParsedInternalKey2DebugSds(&parsed);
    }
    return sdscatprintf("(bad)%s", key->rep);
}