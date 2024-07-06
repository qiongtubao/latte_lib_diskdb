#ifndef __LATTE_DISKDB_WRITE_H
#define __LATTE_DISKDB_WRITE_H

#include "status.h"
#include "db.h"
#include <utils/error.h>

typedef struct _DiskDbWriteOptions
{
    /* data */
} DiskDbWriteOptions;

Error* diskDbPut(DiskDb* db, DiskDbWriteOptions wp,char* key, char* value);


#endif