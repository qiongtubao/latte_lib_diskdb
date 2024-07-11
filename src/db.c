#include "db.h"
#include <zmalloc/zmalloc.h>
#include "versionEdit.h"
#include <fs/dir.h>
#include <assert.h>
#include <string.h>
#include "logWriter.h"
#include "versionEdit.h"
sds global_result = NULL;
DiskDb* diskDbCreate(char* path) {
    DiskDb* db = zmalloc(sizeof(DiskDb));
    mutexInit(&db->mutex);
    db->dbname = sdsnew(path);
    db->env = envCreate();
    return db;
}

void diskDbRelease(DiskDb* db) {
    envRelease(db->env);
    mutexDestroy(&db->mutex);
    sdsfree(db->dbname);
    zfree(db);
}

int isMemEmpty(DiskDb* db) {
    return db->mem != NULL;
}
sds LockFileName(char* dir_name)  {
    return sdscatprintf(sdsempty(), "%s/Lock", dir_name);
}
sds CurrentFileName(char* dir_name) {
    return sdscatprintf(sdsempty(), "%s/CURRENT", dir_name);
}

sds DescriptorFileName(sds dbname, uint64_t number) {
    return sdscatprintf(sdsempty(), "%s/MANIFEST-%06llu",
        dbname,
        number);
}

Error* dbCreate(DiskDb* db) {
    VersionEdit edit;
    versionEditInit(&edit);
    //comparator
    edit.comparator = bytewiseComparator.getName();
    edit.log_number = 0;
    edit.next_file_number = 2;
    edit.last_sequence = 0;

    //  manifest 从1开始
    const sds manifest = DescriptorFileName(db->dbname, 1);
    WritableFile* file;
    Error* error = envNewWritableFile(db->env, manifest, &file);
    if(!isOk(error)) {
        return error;
    }
    {
        LogWriter* writer = writeLogCreate(file);
        sds record = versionEditToSds(&edit);
        Error* error = logAddRecord(writer, record);
        if (isOk(error)) {
            error = writableFileSync(file);
        }
        if (isOk(error)) {
            error = writableFileClose(file);
        }
        sdsfree(record);

    }
    closeWritableFile(file);
    if (isOk(error)) {
        error = dbSetCurrentFile(db, 1);
    } else {
        envRemoveFile(manifest);
    }
    return &Ok;
}
Error* RecoverDb(DiskDb* db, VersionEdit* edit, bool* save_manifest) {
    mutexAssertHeld(&db->mutex);
    Error* error = dirCreate(db->dbname);
    assert(isOk(error));
    sds lockfile = LockFileName(db->dbname);
    error = envLockFile(db->env, lockfile, &db->db_lock);
    sdsfree(lockfile);
    if (!isOk(error)) {
        return error;
    }
    sds currentFileName = CurrentFileName(db->dbname);
    
    if (!fileExists(currentFileName)) {
        //无文件可创建
        if (db->options.create_if_missing) {
            printf("Creating DB %s since it was missing.",
                db->dbname);
            error = dbCreate(db);
            if (!isOk(error)) {
                return error;
            }
        } else {
            return errorCreate(CInvalidArgument ,db->dbname, "does not exist (create_if_missing is false)");
        }
    } else {
        if (db->options.error_if_exists) {
            return errorCreate(CInvalidArgument, db->dbname,
                                        "exists (error_if_exists is true)");
        }
    }
    // error = recoverVersions(db->versions, save_manifest);
    if (!isOk(error)) {
        return error;
    }
    return &Ok;
}


Error* diskDbOpen(DiskDbOptions op, char* path, DiskDb** db) {
    *db = NULL;
    DiskDb* impl = diskDbCreate(path);
    impl->options = op;
    impl->dbname = sdsnew(path);
    mutexLock(&impl->mutex);
    VersionEdit edit;
    bool save_manifest = false;
    //恢复数据
    Error* s = RecoverDb(impl, &edit, &save_manifest);
    if (isOk(s) && impl->mem == NULL) {
        //创建新的Log 和 MemTable对象
        //uint64_t new_log_number = newFileNumber(impl->versions);
    }
    mutexUnlock(&impl->mutex);
    *db = impl;
    return &Ok;
}