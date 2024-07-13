#include "versionEdit.h"
#include "sds/sds_plugins.h"
#include "internalKey.h"
typedef struct Pair {
    void* first;
    void* second;
} Pair;
enum Tag {
  kComparator = 1,  //比较器
  kLogNumber = 2,   //日志文件序列号
  kNextFileNumber = 3,  //下一个文件序列号
  kLastSequence = 4,    //下一个写入序列号
  kCompactPointer = 5,  //CompactPoint类型
  kDeletedFile = 6,     //删除文件
  kNewFile = 7,       //增加的文件
  // 8 was used for large value refs
  kPrevLogNumber = 9  //前一个日志文件序列号
};
sds versionEditToSds(VersionEdit* ve) {
    sds dst = sdsempty();
    if (ve->comparator != NULL) {
        //可变长度的32位整形
        dst = sdsAppendVarint32(dst, kComparator);
        Slice slice;
        slice.len = sdslen(ve->comparator);
        slice.p = ve->comparator;
        dst = sdsAppendLengthPrefixedSlice(dst, &slice);
    }
    if (ve->log_number > 0) {
        dst = sdsAppendVarint32(dst, kLogNumber);
        dst = sdsAppendVarint64(dst, ve->log_number);
    }
    if (ve->prev_log_number > 0) {
        dst = sdsAppendVarint32(dst, kPrevLogNumber);
        dst = sdsAppendVarint64(dst, ve->prev_log_number);
    }
    if (ve->next_file_number > 0) {
        dst = sdsAppendVarint32(dst, kNextFileNumber);
        dst = sdsAppendVarint64(dst, ve->next_file_number);
    }
    if (ve->last_sequence > 0) {
        dst = sdsAppendVarint32(dst, kLastSequence);
        dst = sdsAppendVarint64(dst, ve->last_sequence);
    }
    
    //上次compaction 最大键保存
    listIter* iter = listGetIterator(ve->compact_pointers, AL_START_HEAD);
    listNode* node;
    
    while ((node = listNext(iter)) != NULL) {
        Pair* p = (Pair*)node->value;
        dst = sdsAppendVarint32(dst, kCompactPointer);
        //level
        dst = sdsAppendVarint32(dst, p->first);
        //compaction 操作的最大键值编码
        InternalKey* ikey = (InternalKey*)(p->second);
        dst = sdsAppendLengthPrefixedSlice(dst, ikey->rep);
    }
    listReleaseIterator(iter);

    dictIterator* diter = dictGetIterator(ve->delete_files);
    dictEntry*  de;
    while ((de = dictNext(diter)) != NULL) {
        sds key = (sds)(dictGetEntryKey(de));
        dst = sdsAppendVarint32(dst, kDeletedFile);
        dst = sdsAppendVarint32(dst, key);
        dst = sdsAppendVarint64(dst, de->v.u64);
    }
    dictReleaseIterator(diter);

    iter = listGetIterator(ve->compact_pointers, AL_START_HEAD);
    while ((node = listNext(iter)) != NULL) {
        Pair* p = (Pair*)node->value;
        FileMetaData* f = (FileMetaData*)(p->second);
        dst = sdsAppendVarint32(dst, kNewFile);
        dst = sdsAppendVarint32(dst, p->first);
        dst = sdsAppendVarint64(dst, f->number);
        dst = sdsAppendVarint64(dst, f->file_size);
        dst = sdsAppendLengthPrefixedSlice(dst, f->smallest.rep);
        dst = sdsAppendLengthPrefixedSlice(dst, f->largest.rep);
    }
    listReleaseIterator(iter);
    return dst;
}



void versionEditInit(VersionEdit* ve) {
    ve->log_number = -1;
    ve->comparator = NULL;
    ve->next_file_number = -1;
    ve->prev_log_number = -1;
    ve->last_sequence = -1;
    ve->compact_pointers = listCreate();
    ve->delete_files = setCreate(&sdsSetDictType);
    ve->new_files = listCreate();
}

void versionEditDestroy(VersionEdit* ve) {
    listRelease(ve->compact_pointers);
    listRelease(ve->new_files);
    setRelease(ve->delete_files);
}

VersionEdit* versionEditCreate() {
    VersionEdit* edit = zmalloc(sizeof(VersionEdit));
    versionEditInit(edit);
    return edit;
}

void versionEditRelease(VersionEdit* ve) {
    versionEditDestroy(ve);
    zfree(ve);
}


