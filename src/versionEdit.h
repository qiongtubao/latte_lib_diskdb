
#ifndef __LATTE_DISKDB_VERSIONEDIT_H
#define __LATTE_DISKDB_VERSIONEDIT_H
#include "sds/sds.h"
#include "list/list.h"
#include "sequenceNumber.h"
#include "set/set.h"
#include "internalKey.h"
#include "sds/sds_plugins.h"
#include "utils/error.h"

typedef struct VersionEdit {
    sds comparator;
    uint64_t log_number;
    uint64_t prev_log_number;
    uint64_t next_file_number;
    SequenceNumber last_sequence;
    
    list* compact_pointers;
    set* delete_files;
    list* new_files;
} VersionEdit;

VersionEdit* versionEditCreate();
void versionEditInit(VersionEdit* ve);
void versionEditDestroy(VersionEdit* ve);
sds versionEditToSds(VersionEdit* ve);
Error* decodeVersionEditSlice(VersionEdit* edit, Slice* src);
bool editHasLogNumber(VersionEdit* ve);
bool editHasPrevLogNumber(VersionEdit* ve);
bool editHasNextFileNumber(VersionEdit* ve);
bool editHasLastSequence(VersionEdit* ve);

typedef struct FileMetaData {
    int refs;
    int allowed_seeks;
    uint64_t number;
    uint64_t file_size;
    InternalKey smallest;
    InternalKey largest;
} FileMetaData;
typedef struct Pair {
    void* first;
    void* second;
} Pair;
FileMetaData* fileMetaDataCreate();

#endif