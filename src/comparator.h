#ifndef __LATTE_COMPARATOR_H
#define __LATTE_COMPARATOR_H
#include "sds/sds.h"
#include "internalKey.h"
typedef struct ComparatorSds {
    sds (*getName)();
    int (*compare)(sds* a, sds* b);
    void (*findShortestSeparator)(sds* start, sds limit);
    void (*findShortSuccessor)(sds* key);
} ComparatorSds;

extern ComparatorSds bytewiseComparator;
typedef struct InternalKeyComparator {
    ComparatorSds comparator;
} InternalKeyComparator;

int internalKeyComparatorCompare(InternalKeyComparator* comparator, InternalKey* a, InternalKey* b);


#endif