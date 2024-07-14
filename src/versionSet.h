


#ifndef __LATTE_DISKDB_VERSIONSET_H
#define __LATTE_DISKDB_VERSIONSET_H
#include "fs/env.h"
#include "comparator.h"
#include "logWriter.h"
#include "recover.h"
#include "versionEdit.h"
typedef struct TableCache {

} TableCache;

typedef struct Version {

} Version;
typedef struct VersionSet {
    Env* env;
    sds dbname;
    //DiskDbOptions* options;
    TableCache* tableCache;
    // Comparator comp; //对比器
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

VersionSet* versionSetCreate(sds dbname, Env* env);
void versionSetRelease(VersionSet* versionset);
Error* recoverVersions(VersionSet* set, bool* save_manifest);


typedef struct LevelState {
    set* deleted_files;
    set* added_files;
} LevelState;
typedef struct VersionSetBuilder {
    VersionSet* vset;
    Version* base;
    LevelState levels[7]; //kNumLevels
} VersionSetBuilder;

typedef struct CorruptionReporter {
    WritableFile* dst;
} CorruptionReporter;


typedef struct VersionSetRecover {
    Recover recover;
    Error* error;
} VersionSetRecover;

void VersionSetRecoverCorruption(VersionSetRecover* recover, size_t bytes, Error* s);
typedef struct LogReader {
    SequentialFile* file;
    bool checksum;
    sds backing_store;
    Slice buffer;
    bool eof;
    uint64_t last_record_offset;
    uint64_t end_of_buffer_offset;
    uint64_t initial_offset;
    bool resyncing;
    Recover* revocer;
    char* store;
} LogReader;

typedef enum LogReaderRecordType {
  kEof = 5, //kMaxRecordType + 1
  // Returned whenever we find an invalid physical record.
  // Currently there are three situations in which this happens:
  // * The record has an invalid CRC (ReadPhysicalRecord reports a drop)
  // * The record is a 0-length record (No drop is reported)
  // * The record is below constructor's initial_offset (No drop is reported)
  kBadRecord = 6 //kMaxRecordType + 2
} LogReaderRecordType;
VersionSetBuilder* versionSetBuilderCreate();
bool readLogRecord(LogReader* reader, Slice* record, sds* scratch);
void versionSetBuilderApply(VersionSetBuilder* builer, VersionEdit* edit);
#endif