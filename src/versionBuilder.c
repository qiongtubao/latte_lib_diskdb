#include "versionBuilder.h"
#include "set/avlSet.h"
#include "assert.h"


VersionSetBuilder* versionSetBuilderCreate() {
    VersionSetBuilder* builder = zmalloc(sizeof(VersionSetBuilder));
    return builder;
}

void versionSetBuilderApply(VersionSetBuilder* builer, VersionEdit* edit) {
    listIter* iter = listGetIterator(edit->compact_pointers, AL_START_HEAD);
    listNode* node;
    while ((node = listNext(iter)) != NULL) {
        Pair* p = (Pair*)node->value;
        int level = (p->first);
        builer->vset->compact_pointer[level] = 
            encodeInternalKey(p->second);
    }
    listReleaseIterator(iter);

    dictIterator* diter = dictGetIterator(edit->delete_files);
    dictEntry*  de;
    while ((de = dictNext(diter)) != NULL) {
        Pair* p = (Pair*)dictGetEntryKey(de);
        int level = p->first;
        uint64_t number = p->second;
        setAdd(
            builer->levels[level].deleted_files,
            number);
    }
    dictReleaseIterator(diter);

    iter = listGetIterator(edit->new_files, AL_START_HEAD);
    while ((de = listNext(iter)) != NULL) {
        Pair* p = (Pair*)node->value;
        int level = p->first;
        FileMetaData* f = fileMetaDataCreate(p->second);
        f->refs = 1; 
        // We arrange to automatically compact this file after
        // a certain number of seeks.  Let's assume:
        //   (1) One seek costs 10ms
        //   (2) Writing or reading 1MB costs 10ms (100MB/s)
        //   (3) A compaction of 1MB does 25MB of IO:
        //         1MB read from this level
        //         10-12MB read from next level (boundaries may be misaligned)
        //         10-12MB written to next level
        // This implies that 25 seeks cost the same as the compaction
        // of 1MB of data.  I.e., one seek costs approximately the
        // same as the compaction of 40KB of data.  We are a little
        // conservative and allow approximately one seek for every 16KB
        // of data before triggering a compaction.
        // 我们安排在一定数量的寻道之后自动压缩此文件。我们假设：
        // (1) 一次寻道花费 10ms
        // (2) 写入或读取 1MB 花费 10ms（100MB/s）
        // (3) 压缩 1MB 需要 25MB 的 IO：
        // 从此级别读取 1MB
        // 从下一级别读取 10-12MB（边界可能未对齐）
        // 向下一级别写入 10-12MB
        // 这意味着 25 次寻道的成本与压缩 1MB 数据的成本相同。即，一次寻道的成本大约与压缩 40KB 数据的成本相同。
        // 我们有点保守，在触发压缩之前，大约每 16KB 数据进行一次寻道。
        
        // 文件大小/16k 是允许失败的次数 
        f->allowed_seeks = (int)(f->file_size / 16384U);
        //次数 < 100 设置为100
        if (f->allowed_seeks < 100) f->allowed_seeks = 100;

        setRemove(builer->levels[level].deleted_files, f->number);
        setAdd(builer->levels[level].added_files, f);
    }
    listReleaseIterator(iter);
 
}



void builderSaveToVersion(VersionSetBuilder* builder,Version* version) {
    InternalKeyComparator* internal_comparator = builder->vset->icmp;
    for (int level = 0; level < 7; level++) {
        list* base_files = builder->base->files[level];
        listIter* base_iter = listGetIterator(base_files, AL_START_HEAD);
        set* added_files = builder->levels[level].added_files;
        //version->files[level] 设置 (base_files.size() + added_files->size());
        FileMetaData* added_file;
        Iterator* addIter = setGetIterator(added_files);
        avlNode* anode = NULL;
        listNode* bnode = NULL;
        //sort
        while(iteratorHasNext(addIter)) {
            //base_file是有序set
            //然后重新对比之后按照顺序 versionMaybeAddFile 调用
            //简单来说就是遍历base_file的基础上 按照大小追加added_file
            anode = iteratorNext(addIter);
            FileMetaData* ametadata = (anode->key);

            while((bnode = listNext(base_iter)) != NULL) {
                FileMetaData* bmetadata = listNodeValue(bnode);
                if ( bySmallestKeyOperator(internal_comparator, bmetadata, ametadata) > 0 ) {
                    versionMaybeAddFile(builder, version, level, bmetadata);
                } else {
                    versionMaybeAddFile(builder, version, level, ametadata);
                    versionMaybeAddFile(builder, version, level, bmetadata);
                    break;
                }
            }
        }
        while((bnode = listNext(base_iter)) != NULL) {
            FileMetaData* bmetadata = listNodeValue(bnode);
            versionMaybeAddFile(builder, version, level, bmetadata);
        }
        iteratorRelease(addIter);
        versionMaybeAddFile(builder, version, level, added_file);



        //== 检查顺序是否一致
        if (level > 0) {
            listIter* iter = listGetIterator(version->files, AL_START_HEAD);
            listNode* prev = NULL;
            listNode* current = NULL;
            while ((current = listNext(iter)) != NULL) {
                if (prev == NULL) {
                    prev = current;
                } else {
                    InternalKey prev_end = ((FileMetaData*)(listNodeValue(prev)))->largest;
                    InternalKey this_begin = ((FileMetaData*)(listNodeValue(current)))->smallest;
                    if (internalKeyComparatorCompare(internal_comparator, &prev_end, &this_begin) >= 0) {
                        sds prev_end_sds = internalKey2DebugSds(&prev_end);
                        sds this_begin_sds = internalKey2DebugSds(&this_begin);
                        fprintf(stderr, "overlapping ranges in same level %s vs %s",
                            prev_end_sds, this_begin_sds);
                        sdsfree(prev_end_sds);
                        sdsfree(this_begin_sds);
                        abort();
                    }
                } 

            }
        }
    }

}

void versionMaybeAddFile(VersionSetBuilder* builder, Version* version, int level, FileMetaData* file) {
    if (setContains(builder->levels[level].deleted_files, file->number) == 1) {

    } else {
        list* files = version->files[level];
        if (level > 0 && !(listLength(files) == 0)) {
            // Must not overlap
            int result  =  internalKeyComparatorCompare(builder->vset->icmp, &((FileMetaData*)(listLast(files)))->largest,
                                        &file->smallest);
            assert(result < 0);
            file->refs++;
            files = listAddNodeTail(files, file);
            version->files[level] = files;
        }
    }
}
