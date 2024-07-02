#include "status.h"
#include "db.h"


typedef struct _DiskDbWriteOptions
{
    /* data */
} DiskDbWriteOptions;

DiskDbState diskDbSet(DiskDb* db, DiskDbWriteOptions wp,char* key, char* value);