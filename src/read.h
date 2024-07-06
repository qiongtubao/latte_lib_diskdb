#ifndef __LATTE_DISKDB_READ_H
#define __LATTE_DISKDB_READ_H
#include "status.h"
#include "db.h"
#include <sds/sds.h>
#include <utils/error.h>
typedef struct _DiskDbReadOptions
{
    /* data */
} DiskDbReadOptions;

Error* diskDbGet(DiskDb* db, DiskDbReadOptions rp, char* key, sds* value);
static Error DbNotFindValue = {CNotFound, NULL};
int isNotFindValue(Error* error);
#endif