

typedef enum {
    Ok = 0, //操作正常
    NotFound = 1, //没找到相关项
    Corruption = 2, //异常崩溃
    NotSupported = 3, //不支持
    InvalidArgument = 4, //非法参数
    IOError = 5 //I/O操作错误
} DiskDbState;