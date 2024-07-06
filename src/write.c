#include "write.h"
#include "db.h"

Error* diskDbPut(DiskDb* db, DiskDbWriteOptions wp,char* key, char* value) {
    global_result = sdsnew(value);
    if (global_result == NULL) {
        return errorCreate(CCorruption, "test", "test1");
    }
    return &Ok;
}