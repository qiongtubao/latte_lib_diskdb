
#ifndef __LATTE_DISKDB_H
#define __LATTE_DISKDB_H

#include <stdlib.h>
#include <stdbool.h>
#include "state.h"
#include <sds/sds.h>
typedef struct  _DiskDb {
    
} DiskDb;


typedef struct _DiskDbOptions {
    bool create_if_missing;
} DiskDbOptions;

DiskDbState diskDbOpen(DiskDbOptions op, char* path, DiskDb** db);

#endif