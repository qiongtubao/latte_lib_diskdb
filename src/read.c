
#include "read.h"
#include "db.h"
#include <sds/sds.h>
extern sds global_result;
int isNotFindValue(Error* error) {
    return error == &DbNotFindValue;
}
Error* diskDbGet(DiskDb* db, DiskDbReadOptions rp, char* key, sds* value) {
    if (NULL == global_result) {
        return &DbNotFindValue;
    }
    *value = sdsnewlen(global_result, sdslen(global_result));
    return &Ok;
}