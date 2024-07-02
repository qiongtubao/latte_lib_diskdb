#ifndef __LATTE_DISKDB_READ_H
#define __LATTE_DISKDB_READ_H
#include "state.h"
#include "db.h"
#include <sds/sds.h>

typedef struct _DiskDbReadOptions
{
    /* data */
} DiskDbReadOptions;

DiskDbState diskDbGet(DiskDb* db, DiskDbReadOptions rp, char* key, sds* value);
#endif