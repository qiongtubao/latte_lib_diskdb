#include <test/testhelp.h>
#include <test/testassert.h>
#include "logWriter.h"
#include "versionEdit.h"
#include "comparator.h"
#include <utils/error.h>
#include <sds/sds.h>
#include <fs/file.h>
#include "logReader.h"
#include "logConstant.h"
#include "versionBuilder.h"

sds createlog(
        uint64_t prev_log_number, uint64_t log_number, 
        uint64_t next_file_number, uint64_t last_sequence) {
    VersionEdit edit;
    versionEditInit(&edit);
    //comparator
    edit.comparator = bytewiseComparator.getName();
    edit.prev_log_number = prev_log_number;
    edit.log_number = log_number;
    edit.next_file_number = next_file_number;
    edit.last_sequence = last_sequence;
    
    //versionEdit序列化
    return versionEditToSds(&edit);
    
    
}

uint64_t logs[2][4] = {
    {1,2,3,4},
    {5,6,7,8}
};
char* filename = "test_log";
int test_log_write() {
     Env* env = envCreate();
    WritableFile* file;
    const sds manifest = sdsnew(filename);
    //创建MANIFEST-000001写入文件
    Error* error = envWritableFileCreate(env, manifest, &file);
    LogWriter* writer = writeLogCreate(file);
    for( int i = 0; i <2; i++) {
        sds record = createlog(logs[i][0], logs[i][1], logs[i][2], logs[i][3]);
        //写入到MANIFEST文件
        error = logAddRecord(writer, record);
        //sync MANIFEST
        if (isOk(error)) {
            error = writableFileSync(file);
        } else {
            return 0;
        }
        sdsfree(record);
    }
    //close MANIFEST
    if (isOk(error)) {
        error = writableFileClose(file);
    }
    
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
            // .recover.Corruption = versionSetRecoverCorruption,
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
                assert(edit.log_number == logs[read_records][1]);
                last_log_number = edit.log_number;
                have_log_number = true;
            }

            if (editHasPrevLogNumber(&edit)) {
                assert(edit.prev_log_number == logs[read_records][0]);
                last_prev_log_number = edit.prev_log_number;
                have_prev_log_number = true;
            }

            if (editHasNextFileNumber(&edit)) {
                assert(edit.next_file_number == logs[read_records][2]);
                last_next_file_number = edit.next_file_number;
                have_next_file = true;
            }

            if (editHasLastSequence(&edit)) {
                assert(edit.last_sequence == logs[read_records][3]);
                last_sequence = edit.last_sequence;
                have_last_sequence = true;
            }

            ++read_records;
        }
        assert(read_records == 2);

        // writableFileRelease(file);
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