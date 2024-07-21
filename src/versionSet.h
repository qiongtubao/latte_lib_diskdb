


#ifndef __LATTE_DISKDB_VERSIONSET_H
#define __LATTE_DISKDB_VERSIONSET_H
#include "fs/env.h"
#include "comparator.h"
#include "logWriter.h"
#include "recover.h"
#include "versionEdit.h"
#include "version.h"
#include "list/list.h"
#include "set/set.h"

typedef struct TableCache {

} TableCache;

typedef struct Version Version;
typedef struct VersionSet VersionSet;

typedef struct VersionSet {
    Env* env;
    sds dbname;
    //DiskDbOptions* options;
    TableCache* tableCache;
    // ComparatorSds comp; //对比器
    InternalKeyComparator* icmp;
    uint64_t next_file_number;
    uint64_t manifest_file_number;
    uint64_t last_sequence;
    uint64_t log_number;
    uint64_t prev_log_number;
    WritableFile* descriptor_file;
    LogWriter descriptor_log;
    Version dummy_versions;
    Version* current;
    sds compact_pointer[7]; //kNumLevels  以后弄成动态可扩容
} VersionSet;

VersionSet* versionSetCreate(sds dbname, Env* env, InternalKeyComparator* icmp);
void versionSetRelease(VersionSet* versionset);
Error* recoverVersions(VersionSet* set, bool* save_manifest);


typedef struct CorruptionReporter {
    WritableFile* dst;
} CorruptionReporter;
typedef struct VersionSetRecover {
    Recover recover;
    Error* error;
} VersionSetRecover;

// void versionSetRecoverCorruption(VersionSetRecover* recover, size_t bytes, Error* s);

// Version* versionSet2Version(VersionSet* set);

#endif