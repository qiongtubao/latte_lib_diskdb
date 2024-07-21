#include "versionSet.h"
#include "fileName.h"
#include "crc/crc.h"
#include "versionEdit.h"
#include "set/avlSet.h"
#include "versionBuilder.h"
#include "logReader.h"
#include "fileMetaData.h"
VersionSet* versionSetCreate(sds dbname, Env* env, InternalKeyComparator* icmp) {
    VersionSet* versionset = zmalloc(sizeof(VersionSet));
    versionset->dbname = dbname;
    versionset->icmp = icmp;
    return versionset;
}

void versionSetRelease(VersionSet* versionset) {
    zfree(versionset);
}



void versionSetMakeFileNumberUsed(VersionSet* set, uint64_t number) {
    if (set->next_file_number <= number) {
        set->next_file_number = number + 1;
    }
}

void versionSetAppendVersion(VersionSet* set, Version* v) {
    // Make "v" current
    assert( v -> refs == 0);
    assert( v != set->current);
    if (set->current != NULL) {
        versionUnref(v);
    }
    set->current = v;
    versionRef(v);

    v->prev = set->dummy_versions.prev;
    v->next = &set->dummy_versions;
    v->prev->next = v;
    v->next->prev = v;
}



#define kNumLevels 7
#define kL0_CompactionTrigger 4 
int64_t TotalFileSize(list* files) {
    int64_t sum = 0;
    listIter* iter = listGetIterator(files, AL_START_HEAD);
    listNode* node = NULL;
    while ((node = listNext(iter)) != NULL) {
        FileMetaData* file = listNodeValue(node);
        sum += file->file_size;
    }
    listReleaseIterator(iter);
    return sum;
} 

/**
 * 目前认为每层放大倍数为10
 */
static double MaxBytesForLevel(int level) { 
  // Note: the result for level zero is not really used since we set
  // the level-0 compaction threshold based on number of files.

  // Result for both level-0 and level-1
  double result = 10. * 1048576.0;
  while (level > 1) {
    result *= 10;
    level--;
  }
  return result;
}

//计算分数
void versionSetFinalizeVersion(VersionSet* set, Version* v) {
    // Precomputed best level for next compaction
  int best_level = -1;
  double best_score = -1;

  for (int level = 0; level < kNumLevels - 1; level++) {
    double score;
    if (level == 0) {
      // We treat level-0 specially by bounding the number of files
      // instead of number of bytes for two reasons:
      //
      // (1) With larger write-buffer sizes, it is nice not to do too
      // many level-0 compactions.
      //
      // (2) The files in level-0 are merged on every read and
      // therefore we wish to avoid too many files when the individual
      // file size is small (perhaps because of a small write-buffer
      // setting, or very high compression ratios, or lots of
      // overwrites/deletions).
      // 我们通过限制文件数量而不是字节数来特殊处理级别 0，原因有二：
      //
      // (1) 如果写入缓冲区较大，最好不要进行太多级别 0 压缩。
      //
      // (2) 每次读取时都会合并级别 0 中的文件，因此我们希望在单个文件较小时避免文件过多（可能是因为写入缓冲区设置较小，或者压缩率非常高，或者有很多覆盖/删除）。
      score = listLength(v->files[level]) /
              (double)(kL0_CompactionTrigger);
    } else {
      // Compute the ratio of current size to size limit.
      const uint64_t level_bytes = TotalFileSize(v->files[level]);
      score =
          (double)(level_bytes) / MaxBytesForLevel(level);
    }

    if (score > best_score) {
      //取一个分数最高的
      best_level = level;
      best_score = score;
    }
  }
  //分数最高的层级记录下来 当compaction_score_ >1的时候就进行compaction
  v->compaction_level = best_level;
  v->compaction_score = best_score;
}
//
bool versionSetReuseManifest(VersionSet* vset,sds dscname,
                            sds dscbase) {
    
}


Error* recoverVersions(VersionSet* set, bool* save_manifest) {
    //先去读取CURRENT 文件拿到manifest file 
    // Read "CURRENT" file, which contains a pointer to the current manifest file
    sds current = NULL;
    sds current_path = CurrentFileName(set->dbname);
    //读取CURRENT文件内容
    Error* error = envReadFileToSds(set->env, current_path, &current);
    if (!isOk(error)) {
        return error;
    }
    //长度不为0 或者 最后不是\n 返回错误
    if (sdslen(current) == 0 || current[sdslen(current)- 1] != '\n') {
        return errorCreate(CCorruption, "recoverVersions", "CURRENT file does not end with newline");
    }
    //文件名
    sdssetlen(current, sdslen(current) - 1);
    //获得当前manifest文件 
    sds dscname = sdscatfmt(set->dbname , "/%s", current);
    SequentialFile* file;
    error = envSequentialFileCreate(set->env, dscname, &file);
    if (!isOk(error)) {
        if (isNotFound(error)) {
            Error* err =  errorCreate(CCorruption, "CURRENT points to a non-existent file", error->state);
            errorRelease(error);
            return err;
        }
        return error;
    }
    bool have_log_number = false;
    bool have_prev_log_number = false;
    bool have_next_file = false;
    bool have_last_sequence = false;

    uint64_t last_log_number = 0;
    uint64_t last_prev_log_number = 0;
    uint64_t last_next_file_number = 0;
    uint64_t last_sequence = 0;

    VersionSetBuilder* builder = versionSetBuilderCreate();
    int read_records = 0;
    {
        LogReader reader = {
            .file = file,
            .resyncing = false,
            .checksum = true,
            .buffer = {
                .p = NULL,
                .len = 0
            },
            .eof = false,
            .last_record_offset = 0,
            .end_of_buffer_offset = 0,
            .initial_offset = 0,
        };
        Slice record = {
            .p = NULL,
            .len = 0
        };
        sds scratch;
        //解析versionEdit
        while (readLogRecord(&reader, &record, &scratch) && isOk(error)) {
            VersionEdit edit = {
                .comparator = NULL,
                .delete_files = NULL,
                .new_files = NULL
            };
            error = decodeVersionEditSlice(&edit, &record);
            if (isOk(error)) {
                if (edit.comparator != NULL 
                    && strncmp(edit.comparator, bytewiseComparator.getName(), sdslen(bytewiseComparator.getName())) == 0) {
                error = errorCreate(CInvalidArgument,
                    edit.comparator
                    , "does not match existing comparator "
                    , bytewiseComparator.getName());
                }
            }
            if (isOk(error)) {
                versionSetBuilderApply(builder, &edit);
            }
            
            if (editHasLogNumber(&edit)) {
                last_log_number = edit.log_number;
                have_log_number = true;
            }

            if (editHasPrevLogNumber(&edit)) {
                last_prev_log_number = edit.prev_log_number;
                have_prev_log_number = true;
            }

            if (editHasNextFileNumber(&edit)) {
                last_next_file_number = edit.next_file_number;
                have_next_file = true;
            }

            if (editHasLastSequence(&edit)) {
                last_sequence = edit.last_sequence;
                have_last_sequence = true;
            }

            ++read_records;
            
        }
    }
    // envSequentialFileRelease(file);
    file = NULL;

    if (isOk(error)) {
        if (!have_next_file) {
            error = errorCreate(CCorruption, "recoverVersions", "no meta-nextfile entry in descriptor");
        } else if (!have_log_number) {
            error = errorCreate(CCorruption, "recoverVersions", "no meta-lognumber entry in descriptor");
        } else if (!have_last_sequence) {
            error = errorCreate(CCorruption, "recoverVersions", "no last-sequence-number entry in descriptor");
        }
        if (!have_prev_log_number) {
            last_prev_log_number = 0;
        }
        versionSetMakeFileNumberUsed(set, last_prev_log_number);
        versionSetMakeFileNumberUsed(set, last_log_number);
    }

    if (isOk(error)) {
        Version* v = versionCreate(set);
        builderSaveToVersion(builder, v);
        versionSetFinalizeVersion(set, v);
        versionSetAppendVersion(set, v);
        set->manifest_file_number = last_next_file_number;
        set->next_file_number = last_next_file_number + 1;
        set->last_sequence = last_sequence;
        set->log_number = last_log_number;
        set->prev_log_number = last_prev_log_number;

        if (versionSetReuseManifest(set ,dscname, current)) {

        } else {
            *save_manifest = true;
        }
    } else {
        // Log(options_->info_log, "Error recovering version set with %d records: %s",
        //     read_records, error.c_str());
    }
    return error;
}



