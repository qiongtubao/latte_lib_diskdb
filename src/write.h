#ifndef __LATTE_DISKDB_WRITE_H
#define __LATTE_DISKDB_WRITE_H

#include "state.h"
#include "db.h"


typedef struct _DiskDbWriteOptions
{
    /* data */
} DiskDbWriteOptions;

DiskDbState diskDbPut(DiskDb* db, DiskDbWriteOptions wp,char* key, char* value);


#endif