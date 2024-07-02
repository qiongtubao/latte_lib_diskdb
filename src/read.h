#include "status.h"
#include "db.h"
#include <sds/sds.h>

typedef struct _DiskDbReadOptions
{
    /* data */
} DiskDbReadOptions;

DiskDbState diskDbGet(DiskDb* db, DiskDbReadOptions rp, char* key, sds* value);