#ifndef __LATTE_DISKDB_CONSTANT_H
#define __LATTE_DISKDB_CONSTANT_H
#define kBlockSize  32768

typedef enum RecordType {
  // Zero is reserved for preallocated files
  kZeroType = 0,
  //表示一条记录完整的写到了一个块上
  kFullType = 1,
  //表示该条记录的第一部分
  // For fragments
  kFirstType = 2,
  //表示记录的中间部分
  kMiddleType = 3,
  //表示记录的最后部分 如果修改了记得去修改logWriter中type_crc数组长度  （由于c语言中数组长度必须是常量）
  kLastType = 4 
} RecordType;

// Header is checksum (4 bytes), length (2 bytes), type (1 byte).
static const int kHeaderSize = 4 + 2 + 1;
static const int kMaxRecordType = kLastType;
#endif