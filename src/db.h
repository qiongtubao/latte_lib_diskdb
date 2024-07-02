#include <stdlib.h>
#include <stdbool.h>
#include "status.h"
typedef struct  _DiskDb {
    
} DiskDb;


typedef struct _DiskDbOptions {
    bool create_if_missing;
} DiskDbOptions;

DiskDbState diskDbOpen(DiskDbOptions op, char* path, DiskDb** db);
