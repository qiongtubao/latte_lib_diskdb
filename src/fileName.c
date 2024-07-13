#include "fileName.h"


sds LockFileName(char* dir_name)  {
    return sdscatprintf(sdsempty(), "%s/Lock", dir_name);
}

sds CurrentFileName(char* dir_name) {
    return sdscatprintf(sdsempty(), "%s/CURRENT", dir_name);
}

sds DescriptorFileName(sds dbname, uint64_t number) {
    return sdscatprintf(sdsempty(), "%s/MANIFEST-%06llu",
        dbname,
        number);
}

sds MakeFileName(sds dbname, uint64_t number,
                                const char* suffix) {
  char buf[100];
  int len = snprintf(buf, sizeof(buf), "%s/%06llu.%s",
                dbname, (unsigned long long)(number), suffix);
  return sdsnewlen(buf, len);
}

sds TempFileName(sds dbname, uint64_t number) {
    return MakeFileName(dbname, number, "dbtmp");
}