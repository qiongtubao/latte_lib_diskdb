#ifndef __LATTE_COMPARATOR_H
#define __LATTE_COMPARATOR_H
#include "sds/sds.h"
typedef struct Comparator {
    sds (*getName)();
    int (*compare)(sds a, sds b);
    void (*findShortestSeparator)(sds* start, sds limit);
    void (*findShortSuccessor)(sds* key);
} Comparator;

extern Comparator bytewiseComparator;

#endif