
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
#include "comparator.h"
#include "versionSet.h"
#include "options.h"


typedef struct  _DiskDb {
    latte_mutex mutex;
    MemTable* mem;
    MemTable* imm;
    sds dbname;
    FileLock* db_lock;
    Env* env;
    DiskDbOptions options;
    ComparatorSds* comparator;
    VersionSet* versions;
} DiskDb;


//打开diskDB
Error* diskDbOpen(DiskDbOptions op, char* path, DiskDb** db);
extern sds global_result;
#endif