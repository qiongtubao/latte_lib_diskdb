#ifndef __LATTE_FILEMETADATA_H
#define __LATTE_FILEMETADATA_H
#include "internalKey.h"
#include "comparator.h"
typedef struct FileMetaData {
    int refs;
    int allowed_seeks;
    uint64_t number;
    uint64_t file_size;
    InternalKey smallest;
    InternalKey largest;
} FileMetaData;
FileMetaData* fileMetaDataCreate();
void fileMetaDataRelease();
bool bySmallestKeyOperator(InternalKeyComparator* comp,FileMetaData* f1, FileMetaData* f2);

#endif