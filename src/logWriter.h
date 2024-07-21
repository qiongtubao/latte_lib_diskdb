
#ifndef __LATTE_DISKDB_LOGWRITER_H
#define __LATTE_DISKDB_LOGWRITER_H
#include <fs/env.h>
#include <fs/file.h>


typedef struct LogWriter {
    WritableFile* file;
    int block_offset;
    // crc32c values for all supported record types.  These are
    // pre-computed to reduce the overhead of computing the crc of the
    // record type stored in the header.
    // 所有受支持的记录类型的 crc32c 值。这些值是
    // 预先计算的，以减少计算存储在标头中的
    // 记录类型的 crc 的开销。
    uint32_t type_crc[5]; //kMaxRecordType + 1
} LogWriter;

LogWriter* writeLogCreate(WritableFile* file);
Error* logAddRecord(LogWriter* writer, sds record);

#endif