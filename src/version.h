
#ifndef __LATTE_DISKDB_VERSION_H
#define __LATTE_DISKDB_VERSION_H

#include "list/list.h"
#include "fileMetaData.h"
typedef struct VersionSet VersionSet;
typedef struct Version Version;
typedef struct Version {
    VersionSet* vset; //该版本属于的版本集合 （双向链表）
    Version* next;   // 向前版本的指针
    Version* prev;   // 向后版本的指针
    
    int refs;       // 引用计数
    list** files;    //每个层级所包含的sstable文件，每个文件以一个fileMetaData结构表示
    FileMetaData* file_to_compact; //下次操作compact的文件信息 (seek_compaction)
    int file_to_compact_level; //下次操作compact的层级
    double compaction_score; //compaction_score_ 分数大于1 需要进行compaction （size_compaction）
    int compaction_level;//需要进行compaction操作的层级等级
    //---- 上面是2种出发策略 （优先策略1）
    //1.size_compaction 策略 文件个数或者文件大小超过限制
    //2.seek_compaction 策略 无效读取过多
} Version;
Version* versionCreate(VersionSet* set);
void versionRelease(Version* version);
void versionUnref(Version* version);
void versionRef(Version* version) ;
#endif