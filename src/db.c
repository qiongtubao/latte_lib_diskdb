#include "db.h"
#include <zmalloc/zmalloc.h>
#include "versionEdit.h"
#include <fs/dir.h>
#include <assert.h>
#include <string.h>
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
char* LockFileName(char* dir_name)  {
    return sdscatprintf(sdsempty(), "%s/Lock", dir_name);
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
    return &Ok;
}


Error* diskDbOpen(DiskDbOptions op, char* path, DiskDb** db) {
    *db = NULL;
    DiskDb* impl = diskDbCreate(path);
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