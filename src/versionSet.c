#include "versionSet.h"
#include "fileName.h"
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

//读取日志
bool readLogRecord(LogReader reader, Slice* record, sds* scratch) {

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
        Slice record;
        sds scratch;
        //解析versionEdit
        while (readLogRecord(&reader, &record, &scratch) && isOk(error)) {

        }
    }

    return error;
}