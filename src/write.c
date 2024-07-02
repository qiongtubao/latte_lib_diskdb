#include "write.h"
#include "db.h"
extern sds global_result;
DiskDbState diskDbPut(DiskDb* db, DiskDbWriteOptions wp,char* key, char* value) {
    global_result = sdsnew(value);
    if (global_result != NULL) {
        return Ok;
    }
    return Corruption;
}