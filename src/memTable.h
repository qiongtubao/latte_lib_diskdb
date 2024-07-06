#ifndef __LATTE_DISKDB_MEMTABLE_H
#define __LATTE_DISKDB_MEMTABLE_H

typedef struct MemTable {
    int refs; //引用次数
} MemTable;

#endif