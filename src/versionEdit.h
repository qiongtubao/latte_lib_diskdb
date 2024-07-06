
#ifndef __LATTE_DISKDB_VERSIONEDIT_H
#define __LATTE_DISKDB_VERSIONEDIT_H

typedef uint64_t SequenceNumber;
typedef struct VersionEdit {
    uint64_t log_number;
    uint64_t prev_log_number;
    uint64_t next_file_number;
    SequenceNumber seq;
    sds comparator;
} VersionEdit;

#endif