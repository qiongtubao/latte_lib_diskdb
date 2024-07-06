
#ifndef __LATTE_DISKDB_H
#define __LATTE_DISKDB_H

#include <stdlib.h>
#include <stdbool.h>
#include "status.h"
#include <sds/sds.h>
#include <mutex/mutex.h>
#include "memTable.h"
#include <fs/file.h>
#include <utils/error.h>
#include <fs/env.h>

typedef struct  _DiskDb {
    latte_mutex mutex;
    MemTable* mem;
    MemTable* imm;
    sds dbname;
    FileLock* db_lock;
    Env* env;
} DiskDb;


typedef struct _DiskDbOptions {
    bool create_if_missing;
} DiskDbOptions;

Error* diskDbOpen(DiskDbOptions op, char* path, DiskDb** db);
extern sds global_result;
#endif