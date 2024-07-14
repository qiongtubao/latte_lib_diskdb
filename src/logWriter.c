#include "logWriter.h"
#include "assert.h"
#include "crc/crc.h"
#include "sds/sds_plugins.h"

static uint32_t type_crc[5] = {0};
void InitTypeCrc(uint32_t* type_crc) {
  for (int i = 0; i <= kMaxRecordType; i++) {
    char t = (char)(i);
    type_crc[i] = crc32c(&t, 1);
  }
}
LogWriter* writeLogCreate(WritableFile* file) {
    LogWriter* logWriter = zmalloc(sizeof(LogWriter));
    logWriter->file = file;
    logWriter->block_offset = 0;
    memset(logWriter->type_crc, 0, (kMaxRecordType + 1) * sizeof(uint32_t));
    InitTypeCrc(&logWriter->type_crc);
    return logWriter;
}

//发送物理记录
Error* writerEmitPhysicalRecord(LogWriter* writer, RecordType t, const char* ptr,
                                size_t length) {
    assert(length <= 0xffff);
    assert(writer->block_offset + kHeaderSize + length <= kBlockSize);
    // Format the header
    // 格式化开头 7个字节
    char buf[kHeaderSize];
    //4,5 保存长度
    buf[4] = (length & 0xff);
    buf[5] = (length >> 8);
    //6 保存类型
    buf[6] = (t);    
    // Compute the crc of the record type and the payload.
    // 计算记录类型和有效载荷的 crc。
    // 通过计算crc   
    //type_crc[t]先计算好crc 保存了 就可以追加其他数据
    // crc32c([0..n]) = crc32extend(crc32[0], crc32c(1..n))
    uint32_t crc = crc32c_extend(writer->type_crc[t], ptr, length);
    crc = crc32c_mask(crc);
    //buf 前4位存放crc
    encodeFixed32(buf, crc);     

    // Write the header and the payload
    // 文件写入 header    
    Error* error = writableFileAppend(writer->file, buf, kHeaderSize);
    if (isOk(error)) {
        //写入数据
        error = writableFileAppend(writer->file, ptr, length);
        if (isOk(error)) {
            //刷到磁盘
            error = writableFileFlush(writer->file);
        }
    } 
    //修改offset
    writer->block_offset += kHeaderSize + length;
    return error;           
}
/**
 * 案例1
 *  记录大小500字节，原来已经有1000字节
 *  总共1000 + 7 + 500
 * 案例2
 *   记录大小31755字节，原来已经有1000字节
 *  1000 + 7 + 31755 = 32762  还剩下6个字节  设置\x00 * 6
 * 案例3
 *   记录50000字节 原来已经有1000字节
*    1000 + 7 + 31761 = 32768
 *  7 + （50000 - 31761 = 18239） = 18246 
 */
//方法将记录写入一个Slice结构
Error* logAddRecord(LogWriter* writer, sds record) {
    //ptr指向需要写入的记录内容
    const char* ptr = record;
    //left代表需要写入的记录内容长度
    size_t left = sdslen(record);
    // Fragment the record if necessary and emit it.  Note that if slice
    // is empty, we still want to iterate once to emit a single
    // zero-length record
    // 如果需要，对记录进行分段并发出。请注意，如果切片
    // 为空，我们仍然希望迭代一次以发出单个
    // 零长度记录
    Error* error;
    bool begin = true;
    do {
        //一个块的大小（32768字节），block_offset_ 代表当前块的写入偏移量
        //因此leftover表明当前块还剩余多少字节可用
        const int leftover = kBlockSize - writer->block_offset;
        assert(leftover >= 0);
        //剩余内容少于7 （header）剩余内容填充\x00
        if (leftover < kHeaderSize) {
            // Switch to a new block
            if (leftover > 0) {
                // Fill the trailer (literal below relies on kHeaderSize being 7)
                // assert(kHeaderSize == 7, "");
                writableFileAppend(writer, "\x00\x00\x00\x00\x00\x00", leftover);
            }
            writer->block_offset = 0;
        }
        assert(kBlockSize - writer->block_offset - kHeaderSize >= 0);
        //本块block剩余可用
        const size_t avail = kBlockSize - writer->block_offset - kHeaderSize;
        //本块写入的长度 （数据长度和可用值 取最小值）
        const size_t fragment_length = (left < avail) ? left : avail;

        RecordType type;
        //是否能在本块写完
        const bool end = (left == fragment_length);
        if (begin && end) {
            //开始和结束都在同一块  类型就是kFullType
            type = kFullType;
        } else if (begin) {
            // 如果本次开始的话 类型就是kFirstType
            type = kFirstType;
        } else if (end) {
            // 如果本次结束的话 类型就是kLastType
            type = kLastType;
        } else {
            // 其他 类型就是kMiddleType
            type = kMiddleType;
        }
        //写入数据
        error = writerEmitPhysicalRecord(writer, type, ptr, fragment_length);
        //修改数据指针
        ptr += fragment_length;
        //修改剩余长度
        left -= fragment_length;
        begin = false;
    } while (isOk(error) && left > 0);
    
    return error;

}