#ifndef __LATTE_DISKDB_RECOVER_H
#define __LATTE_DISKDB_RECOVER_H
#include "utils/error.h"
#include "recover.h"

typedef struct Recover {
    void (*Corruption)(void* recover, size_t bytes, Error* s);
} Recover;


#endif

