#include "versionEdit.h"
#include "sds/sds_plugins.h"
#include "internalKey.h"
#include "utils/error.h"

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
        printf("\n1.%lld\n", ve->log_number);
        dst = sdsAppendVarint32(dst, kLogNumber);
        dst = sdsAppendVarint64(dst, ve->log_number);
    }
    if (ve->prev_log_number > 0) {
        printf("2.%lld\n", ve->prev_log_number);
        dst = sdsAppendVarint32(dst, kPrevLogNumber);
        dst = sdsAppendVarint64(dst, ve->prev_log_number);
    }
    if (ve->next_file_number > 0) {
        printf("3.%lld\n", ve->next_file_number);
        dst = sdsAppendVarint32(dst, kNextFileNumber);
        dst = sdsAppendVarint64(dst, ve->next_file_number);
    }
    if (ve->last_sequence > 0) {
        printf("4.%lld\n", ve->last_sequence);
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

#define kNumLevels 7

bool getLevel(Slice *input, int *level) {
    uint32_t v;
    if (getVarint32(input, &v) && v < kNumLevels) {
        *level = v;
        return true;
    } else {
        return false;
    }
}

bool decodeInternalKey(Slice* slice, InternalKey* dst) {
    dst->rep = sdsnewlen(slice->p, slice->len);
    return sdslen(dst->rep) != 0;
}

bool getInternalKey(Slice *input, InternalKey *dst) {
    Slice str;
    if (getLengthPrefixedSlice(input, &str)) {
        return decodeInternalKey(&str, dst);
    } else {
        return false;
    }
}
Pair* pairCreate(void* key, void* value) {
    Pair* pair = zmalloc(sizeof(Pair));
    pair->first = key;
    pair->second = value;
    return pair;
}

void editClear(VersionEdit* edit) {
   
    if (edit->comparator != NULL) {
        sdsfree(edit->comparator);
    }

    if (edit->delete_files != NULL) {
        setRelease(edit->delete_files);
    } 

    if (edit->new_files != NULL) {
        listRelease(edit->new_files);
    }
    
    
    versionEditInit(edit);

}
//字符串解析成VersionEdit
Error* decodeVersionEditSlice(VersionEdit* edit, Slice* src) {
    editClear(edit);
    Slice input = {
        .p = src->p,
        .len = src->len
    };
    const char* msg = NULL;
    uint32_t tag;

    int level;
    uint64_t number;
    FileMetaData f;
    Slice str;
    InternalKey key;
    printf("input :%s %d\n", input.p, input.len);
    while (msg == NULL && getVarint32(&input, &tag)) {
        printf("tag: %d\n", tag);
        switch (tag)
        {
            case kComparator:
                /* code */
                if (getLengthPrefixedSlice(&input, &str)) {
                    edit->comparator = str.p;
                } else {
                    msg = "comparator name";
                }
                break;
            case kLogNumber:
                if (getVarint64(&input, &edit->log_number)) {
                    printf("kLogNumber...%lld\n", edit->log_number);
                } else {
                    msg = "log number";
                }
                break;
            case kPrevLogNumber:
                if (getVarint64(&input, &edit->prev_log_number)) {
                    printf("kPrevLogNumber...%lld\n", edit->prev_log_number);
                } else {
                    msg = "previous log number";
                }
                break;
            case kNextFileNumber:
                if (getVarint64(&input, &edit->next_file_number)) {
                    printf("kNextFileNumber...%lld\n", edit->next_file_number);
                } else {
                    msg = "next file number";
                }
                break;
            case kLastSequence:
                if (getVarint64(&input, &edit->last_sequence)) {
                    printf("kLastSequence...%lld\n", edit->last_sequence);
                } else {
                    msg = "last sequence number";
                }
                break;
            case kCompactPointer:
                if (getLevel(&input, &level) && getInternalKey(&input, &key)) {
                    InternalKey* copy = internalKeyCopy(&key);
                    listAddNodeTail(edit->compact_pointers, pairCreate(level, &key));
                } else {
                    msg = "compaction pointer";
                }
                break;
            case kDeletedFile:
                if (getLevel(&input, &level) && getVarint64(&input, &number)) {
                    setAdd(edit->delete_files, pairCreate(level, number));
                } else {
                    msg = "deleted file";
                }
                break;
            case kNewFile:
                if (getLevel(&input, &level)&& getVarint64(&input, &f.number) &&
                    getVarint64(&input, &f.file_size) &&
                    getInternalKey(&input, &f.smallest) &&
                    getInternalKey(&input, &f.largest)) {
                    listAddNodeTail(edit->new_files, pairCreate(level, &f));
                } else {
                    msg = "new-file entry";
                }
                break;

            default:
                msg = "unknown tag";
                break;
        }
    }
    if (msg == NULL && !SliceIsEmpty(&input)) {
        msg = "invalid tag";
    }

    Error* error;
    if (msg != NULL) {
        error = errorCreate(CCorruption, "VersionEdit", msg);
    }
    return error;
}

bool editHasLogNumber(VersionEdit* ve) {
    return ve->log_number > 0;
}
bool editHasPrevLogNumber(VersionEdit* ve) {
    return ve->prev_log_number > 0;
}
bool editHasNextFileNumber(VersionEdit* ve) {
    return ve->next_file_number > 0;
}
bool editHasLastSequence(VersionEdit* ve) {
    return ve->last_sequence > 0;
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