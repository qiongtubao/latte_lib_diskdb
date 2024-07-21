#ifndef __LATTE_DISKDB_LOGWREAD_H
#define __LATTE_DISKDB_LOGWREAD_H
#include "sds/sds.h"
#include "sds/sds_plugins.h"

#include "fs/file.h"
#include "recover.h"

typedef enum LogReaderRecordType {
  kEof = 5, //kMaxRecordType + 1
  // Returned whenever we find an invalid physical record.
  // Currently there are three situations in which this happens:
  // * The record has an invalid CRC (ReadPhysicalRecord reports a drop)
  // * The record is a 0-length record (No drop is reported)
  // * The record is below constructor's initial_offset (No drop is reported)
  kBadRecord = 6 //kMaxRecordType + 2
} LogReaderRecordType;
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

bool readLogRecord(LogReader* reader, Slice* record, sds* scratch);
#endif