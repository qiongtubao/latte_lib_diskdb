
#include "read.h"
#include "db.h"
#include <sds/sds.h>
extern sds global_result;
DiskDbState diskDbGet(DiskDb* db, DiskDbReadOptions rp, char* key, sds* value) {
    if (NULL == global_result) {
        return NotFound;
    }
    *value = sdsnewlen(global_result, sdslen(global_result));
    return Ok;
}