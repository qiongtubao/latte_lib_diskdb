#ifndef __LATTE_DISKDB_SEQUENCE_NUMBER_H
#define __LATTE_DISKDB_SEQUENCE_NUMBER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
typedef uint64_t SequenceNumber;
// We leave eight bits empty at the bottom so a type and sequence#
// can be packed together into 64-bits.
static const SequenceNumber kMaxSequenceNumber = ((0x1ull << 56) - 1);
#endif