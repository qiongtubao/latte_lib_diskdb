#include "fileMetaData.h"

bool bySmallestKeyOperator(InternalKeyComparator* internal_comparator,FileMetaData* f1, FileMetaData* f2) {
    int r = internalKeyComparatorCompare(internal_comparator, &f1->smallest, &f2->smallest);
    if (r != 0) {
        return (r < 0);
    } else {
        return (f1->number < f2->number);
    }
}


FileMetaData* fileMetaDataCreate() {
    FileMetaData* meta = zmalloc(sizeof(FileMetaData));
    meta->refs = 0;
    meta->allowed_seeks = (1<<30);
    meta->file_size = 0;
    meta->smallest.rep = NULL;
    meta->smallest.seq = 0;
    meta->largest.rep = NULL;
    meta->largest.seq = 0;
    return meta;
}
void fileMetaDataRelease(FileMetaData* f) {
    internalKeyDestroy(&f->smallest);
    internalKeyDestroy(&f->largest);
    zfree(f);
}




