#ifndef __LATTE_FILENAME_H
#define __LATTE_FILENAME_H
#include "sds/sds.h"

sds LockFileName(char* dir_name);
sds CurrentFileName(char* dir_name);
sds DescriptorFileName(sds dbname, uint64_t number);
sds MakeFileName(sds dbname, uint64_t number,
                                const char* suffix);
sds TempFileName(sds dbname, uint64_t number);
#endif