#include <test/testhelp.h>
#include <test/testassert.h>
#include "logWriter.h"
#include "versionEdit.h"
#include "comparator.h"
#include <utils/error.h>
#include <sds/sds.h>
#include <fs/file.h>
int test_log_write() {
    Env* env = envCreate();
    VersionEdit edit;
    versionEditInit(&edit);
    //comparator
    edit.comparator = bytewiseComparator.getName();
    edit.prev_log_number = 1;
    edit.log_number = 2;
    edit.next_file_number = 3;
    edit.last_sequence = 4;
    WritableFile* file;
    const sds manifest = sdsnew("test_log");
    //创建MANIFEST-000001写入文件
    Error* error = envWritableFileCreate(env, manifest, &file);
    LogWriter* writer = writeLogCreate(file);
    //versionEdit序列化
    sds record = versionEditToSds(&edit);
    //写入到MANIFEST文件
    error = logAddRecord(writer, record);
    //sync MANIFEST
    if (isOk(error)) {
        error = writableFileSync(file);
    }
    //close MANIFEST
    if (isOk(error)) {
        error = writableFileClose(file);
    }
    sdsfree(record);
    return 1;
}
#include "versionSet.h"
int test_log_read() {
    SequentialFile* file;
    Env* env = envCreate();
    sds filename = sdsnew("test_log");
    
    Error* error = envSequentialFileCreate(env, filename, &file);
    assert(isOk(error));
    VersionSetBuilder* builder = versionSetBuilderCreate();
    uint64_t last_log_number = 0;
    uint64_t last_prev_log_number = 0;
    uint64_t last_next_file_number = 0;
    uint64_t last_sequence = 0;
    bool have_log_number = false;
    bool have_prev_log_number = false;
    bool have_next_file = false;
    bool have_last_sequence = false;
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
            .store = zmalloc(kBlockSize)
        };
        VersionSetRecover recover = {
            .recover.Corruption = VersionSetRecoverCorruption,
            .error = &Ok
        };
        reader.revocer = &recover;
        Slice record;
        //缓存
        sds scratch = sdsemptylen(kBlockSize);
        //读取versionEdit
        // printf("\ntest\n");
        assert(isOk(error));
        while (readLogRecord(&reader, &record, &scratch) && isOk(recover.error)) {
            ++read_records;
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
            printf("???\n");
            if (editHasPrevLogNumber(&edit)) {
                last_prev_log_number = edit.prev_log_number;
                have_prev_log_number = true;
            }
            printf("???1\n");
            if (editHasNextFileNumber(&edit)) {
                last_next_file_number = edit.next_file_number;
                have_next_file = true;
            }
            printf("???2\n");
            if (editHasLastSequence(&edit)) {
                last_sequence = edit.last_sequence;
                have_last_sequence = true;
            }
            printf("???3\n");
        }
        printf("???11111\n");
        // writableFileRelease(file);
        printf("%lld %lld %lld %lld\n", 
            last_log_number, last_next_file_number, 
            last_prev_log_number, last_sequence);
        file = NULL;
        if (isOk(error)) {
            if (!last_next_file_number) {
                error = errorCreate(CCorruption,"loadVersionEdit", "no meta-nextfile entry in descriptor");
            } else if (!have_log_number) {
                error = errorCreate(CCorruption, "loadVersionEdit", "no meta-lognumber entry in descriptor");
            } else if (!have_last_sequence) {
                error = errorCreate(CCorruption, "loadVersionEdit", "no last-sequence-number entry in descriptor");
            }
            if (!have_prev_log_number) {
                last_prev_log_number = 0;
            }

            // MarkFileNumberUsed(versionSet, last_prev_log_number);
            // MarkFileNumberUsed(versionSet, last_log_number);
        }
        printf("\nerror :%s\n",recover.error->state);
    }
    return 1;
}

int test_api(void) {
    {
        test_cond("log write function",
            test_log_write() == 1);
        test_cond("log read function",
            test_log_read() == 1);
    } test_report()
    return 1;
}

int main() {
    test_api();
    return 0;
}