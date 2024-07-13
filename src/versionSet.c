#include "versionSet.h"
#include "fileName.h"
#include "crc/crc.h"

VersionSet* versionSetCreate(sds dbname, Env* env) {
    VersionSet* versionset = zmalloc(sizeof(VersionSet));
    versionset->dbname = dbname;
    return versionset;
}

void versionSetRelease(VersionSet* versionset) {
    zfree(versionset);
}

VersionSetBuilder* versionSetBuilderCreate() {
    VersionSetBuilder* builder = zmalloc(sizeof(VersionSetBuilder));
    return builder;
}



void VersionSetRecoverCorruption(VersionSetRecover* recover, size_t bytes, Error* s) {
    if (isOk(recover->error)) {
        //只替换成Ok的error  如果不是Ok的话 不替换 记录第一个错误
        recover->error = s;
    } 
}

void ReportCorruption(LogReader* logReader,uint64_t bytes, const char* reason) {
    ReportDrop(logReader, bytes, errorCreate(CCorruption, "logReader", reason));
}
void ReportDrop(LogReader* logReader,uint64_t bytes, Error* reason) {
  if (logReader->revocer != NULL &&
      logReader->end_of_buffer_offset - logReader->buffer.len - bytes >= 
      logReader->initial_offset) {
    logReader->revocer->Corruption(logReader->revocer,(size_t)(bytes), reason);
  }
}

unsigned int ReadPhysicalRecord(LogReader* reader, Slice* result) {
    while (true) {
        if (reader->buffer.len < kHeaderSize) {
            //长度小于7
            if (!reader->eof) {
                // Last read was a full read, so this is a trailer to skip
                // 上次读取是完整读取，因此这是一段可以跳过的预告片
                // 清理缓存
                SliceClear(&reader->buffer);
                //buffer读取32768字节， 顺序读
                Error* error = sequentialFileRead(reader->file,
                    kBlockSize, &(reader->buffer));
                //保存读取到那个offset保存
                reader->end_of_buffer_offset += reader->buffer.len;
                if (!isOk(error)) {
                    //失败 清理buffer
                    SliceClear(&reader->buffer);
                    //记录错误
                    ReportDrop(reader->revocer,kBlockSize, error);
                    //设置eof为true
                    reader->eof = true;
                    return kEof;
                } else if (reader->buffer.len < kBlockSize) {
                    //设置eof为true
                    reader->eof = true;
                }
                continue;
            } else {
                // 请注意，如果 buffer_ 非空，则文件末尾的标头会被截断，
                // 这可能是由于写入程序在写入标头的过程中崩溃造成的。
                // 不要将此视为错误，而只需报告 EOF。
                SliceClear(&reader->buffer);
                return kEof;
            }
        }
        //解析头
        char* header = reader->buffer.p;
        //读取4,5 获得长度
        const uint32_t a = (uint32_t)(header[4]) & 0xff;
        const uint32_t b = (uint32_t)(header[5]) & 0xff;
        //6 获得类型
        const unsigned int type = header[6];
        //合并长度
        const uint32_t length = a | (b << 8);
        if (kHeaderSize + length > reader->buffer.len) {
            //7 + 长度 大于buffer的长度
            size_t drop_size = reader->buffer.len;
            SliceClear(&reader->buffer);
            if (!reader->eof) {
                //记录错误长度
                ReportCorruption(reader, drop_size, "bad record length");
                return kBadRecord;
            }
            // 如果到达文件末尾而未读取有效负载的 |length| 字节
            // ，则假设写入器在写入记录的过程中死亡。
            // 不报告损坏。
            return kEof;
        }

        if (type == kZeroType && length == 0) {
            // 跳过零长度记录而不报告任何丢失，因为
            // 此类记录是由 env_posix.cc 中基于 mmap 的写入代码生成的，
            // 该代码预分配文件区域。
            SliceClear(&reader->buffer);
            return kBadRecord;
        }
        // check crc
        // 检查crc
        if (reader->checksum) {
            uint32_t expected_crc = crc32c_unmask(decodeFixed32(header));
            uint32_t actual_crc = crc32c(header + 6, 1 + length);
            if (actual_crc != expected_crc) {
                // 删除缓冲区的其余部分，因为“长度”本身可能
                // 已被损坏，如果我们相信它，我们可以找到一些
                // 真实日志记录的片段，这些片段恰好看起来像
                // 有效的日志记录。
                size_t drop_size = reader->buffer.len;
                SliceClear(&reader->buffer);
                ReportCorruption(reader, drop_size, "checksum mismatch");
                return kBadRecord;
            }
        }
        //buffer后移 一个数据段
        SliceRemovePrefix(&reader->buffer, kHeaderSize + length);
        // 跳过在 initial_offset_ 之前开始的物理记录
        if (reader->end_of_buffer_offset - reader->buffer.len - kHeaderSize - length <
            reader->initial_offset) {
            SliceClear(result);
            return kBadRecord;
        }
        //读取数据
        result->p = sdsnew(header + kHeaderSize);
        result->len = length;
        return type;
    }
}

bool skipToInitialBlock(LogReader* reader) {
    const size_t offset_in_block = reader->initial_offset % kBlockSize;
    uint64_t block_start_location = reader->initial_offset - offset_in_block;

    //如果我们在拖车里，就不要搜索街区
    if (offset_in_block > kBlockSize - 6) {
        block_start_location += kBlockSize;
    }

    reader->end_of_buffer_offset = block_start_location;
    // 跳至可以包含初始记录的第一个块的开头
    if (block_start_location > 0) {
        Error* skip_error = sequentialFileSkip(reader->file, block_start_location);
        if (!isOk(skip_error)) {
            ReportDrop(reader->revocer, block_start_location , skip_error);
            return false;
        }
    }
    return true;
}

//读取日志
//数据存入到record 如果多个block的保存scratch
bool readLogRecord(LogReader* reader, Slice* record, sds* scratch) {
    //最后一次记录offset 小于初始化值 
    if (reader->last_record_offset < reader->initial_offset) {
        if (!skipToInitialBlock(reader)) {
            return false;
        }
    }
    //清理数据
    scratch = sdsemptylen(100);
    //碎片记录
    bool in_fragmented_record = false;
    // Record offset of the logical record that we're reading
    // 0 is a dummy value to make compilers happy
    // 记录我们正在读取的逻辑记录的偏移量
    // 0 是一个虚拟值，以使编译器满意
    uint64_t prospective_record_offset = 0;
    //分段
    Slice fragment;
    while (true) {
        //解析log 数据在fragment
        const unsigned int record_type = ReadPhysicalRecord(reader, &fragment);
        // ReadPhysicalRecord may have only had an empty trailer remaining in its
        // internal buffer. Calculate the offset of the next physical record now
        // that it has returned, properly accounting for its header size.
        // ReadPhysicalRecord 的内部缓冲区中可能只剩下一个空的尾部。
        // 现在计算它返回的下一个物理记录的偏移量，并正确计算其标头大小。
        uint64_t physical_record_offset =
            reader->end_of_buffer_offset - reader->buffer.len - kHeaderSize - fragment.len;
        
        if (reader->resyncing) {
            //如果是同步的话 可以跳过？
            if (record_type == kMiddleType) {
                continue;
            } else if (record_type == kLastType) {
                reader->resyncing = false;
                continue;
            } else {
                reader->resyncing = false;
            }
        }
        switch (record_type) {
            case kFullType:
                if (in_fragmented_record) {
                    // Handle bug in earlier versions of log::Writer where
                    // it could emit an empty kFirstType record at the tail end
                    // of a block followed by a kFullType or kFirstType record
                    // at the beginning of the next block.
                    if (sdslen(scratch) == 0) {
                        ReportCorruption(reader, sdslen(scratch), "partial record without end(1)");
                    }
                }
                prospective_record_offset = physical_record_offset;
                //清理临时
                sdsclear(scratch);
                *record = fragment;
                reader->last_record_offset = prospective_record_offset;
                return true;
            case kFirstType:
                if (in_fragmented_record) {
                    // 处理早期版本的 log::Writer 中的错误，其中
                    // 它可能在块的尾端发出一个空的 kFirstType 记录
                    // 然后在下一个块的开头发出一个 kFullType 或 kFirstType 记录。
                    if (sdslen(scratch) != 0) {
                        ReportCorruption(reader, sdslen(scratch), "partial record without end(2)");
                    }
                }
                //偏移
                prospective_record_offset = physical_record_offset;
                //设置数据
                // scratch->assign(fragment.p, fragment.len);
                scratch = sdsReset(scratch, fragment.p, fragment.len);
                //打开碎片记录
                in_fragmented_record = true;
                break;  
            case kMiddleType:
                if (!in_fragmented_record) {
                    ReportCorruption(reader, fragment.len,
                                "missing start of fragmented record(1)");
                } else {
                    //追加数据
                    scratch = sdscatlen(scratch, fragment.p, fragment.len);
                }
                break;
            case kLastType:
                if (!in_fragmented_record) {
                    ReportCorruption(reader, fragment.len,
                           "missing start of fragmented record(2)");
                } else {
                    //追加数据
                    scratch = sdscatlen(scratch, fragment.p, fragment.len);
                    //合并所有碎片
                    record->p = scratch;
                    record->len = sdslen(scratch);
                    //保存最后的偏移量
                    reader->last_record_offset = prospective_record_offset;
                    return true;
                }
                break;
            case kEof:
                if (in_fragmented_record) {
                    // This can be caused by the writer dying immediately after
                    // writing a physical record but before completing the next; don't
                    // treat it as a corruption, just ignore the entire logical record.
                    // 这可能是由于写入器在写入物理记录之后但在完成下一个记录之前立即死亡所致；不要
                    // 将其视为损坏，而应忽略整个逻辑记录。
                    sdsclear(scratch);
                }
                //返回失败
                return false;
            case kBadRecord:
                if (in_fragmented_record) {
                    ReportCorruption(reader, sdslen(scratch), "error in middle of record");
                    in_fragmented_record = false;
                    sdsclear(scratch);
                }
                break;
            default: {
                //记录类型无识别
                char buf[40];
                snprintf(buf, sizeof(buf), "unknown record type %u", record_type);
                ReportCorruption(reader,
                    (fragment.len + (in_fragmented_record ? sdslen(scratch) : 0)),
                    buf);
                in_fragmented_record = false;
                sdsclear(scratch);
                break;
            }
        }
    }
    return false;
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

    uint64_t next_file = 0;
    uint64_t last_sequence = 0;
    uint64_t log_number = 0;
    uint64_t prev_log_number = 0;

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

        }
    }

    return error;
}