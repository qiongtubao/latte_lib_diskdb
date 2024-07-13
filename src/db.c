#include "db.h"
#include <zmalloc/zmalloc.h>
#include "versionEdit.h"
#include <fs/dir.h>
#include <assert.h>
#include <string.h>
#include "logWriter.h"
#include "versionEdit.h"
#include "fileName.h"
sds global_result = NULL;
DiskDb* diskDbCreate(char* path) {
    DiskDb* db = zmalloc(sizeof(DiskDb));
    mutexInit(&db->mutex);
    db->dbname = sdsnew(path);
    db->env = envCreate();
    db->versions = versionSetCreate(db->dbname, db->env);
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


#include <assert.h>
#include "sds/sds_plugins.h"
Error* dbSetCurrentFile(DiskDb* db ,Env* env, sds dbname,
                      uint64_t descriptor_number) {
  // Remove leading "dbname/" and add newline to manifest file name
  sds manifest = DescriptorFileName(dbname, descriptor_number);
  size_t len = sdslen(manifest);
  char path[len + 1];
  memcpy(path, manifest, len);
  path[len] = '/';
  assert(0 == sdsStartsWith(manifest, path));
  path[len] = '\n';
  //   assert(0 == sdsStartsWith(manifest, dbname + "/"));
  Slice contents = {
    .p = path,
    .len = len + 1
  };
  SliceRemovePrefix(&contents, sdslen(dbname) + 1);
  sds tmp = TempFileName(dbname, descriptor_number);
  Error* error = envWriteSdsToFileSync(env, &contents, tmp);
  if (isOk(error)) {
    error = envRenameFile(env, tmp, CurrentFileName(dbname));
  }
  if (!isOk(error)) {
    envRemoveFile(env, tmp);
  }
  return error;
}

Error* dbCreate(DiskDb* db) {
    //初始化db
    VersionEdit edit;
    versionEditInit(&edit);
    //comparator
    edit.comparator = bytewiseComparator.getName();
    edit.log_number = 0;
    edit.next_file_number = 2;
    edit.last_sequence = 0;
    //manifest 从1开始
    //MANIFEST-000001
    const sds manifest = DescriptorFileName(db->dbname, 1);
    WritableFile* file;
    //创建MANIFEST-000001写入文件
    Error* error = envWritableFileCreate(db->env, manifest, &file);
    if(!isOk(error)) {
        return error;
    }
    {
        LogWriter* writer = writeLogCreate(file);
        //versionEdit序列化
        sds record = versionEditToSds(&edit);
        //写入到MANIFEST文件
        Error* error = logAddRecord(writer, record);
        //sync MANIFEST
        if (isOk(error)) {
            error = writableFileSync(file);
        }
        //close MANIFEST
        if (isOk(error)) {
            error = writableFileClose(file);
        }
        sdsfree(record);
    }
    envWritableFileRelease(db->env, file);
    if (isOk(error)) {
        //通过数字1 重新创建MANIFEST-000001 写入CURRENT
        error = dbSetCurrentFile(db, db->env, db->dbname, 1);
    } else {
        //删除manifest文件
        envRemoveFile(db->env, manifest);
    }
    return error;
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
    //当前CURRENT 文件名
    sds currentFileName = CurrentFileName(db->dbname);
    //判断是否存在CURRENT存在
    if (!fileExists(currentFileName)) {
        //不存在的话创建CURRENT
        if (db->options.create_if_missing) {
            printf("Creating DB %s since it was missing.",
                db->dbname);
            //创建CURRENT 
            //versionEdit写入Manifest-000001 并且写入CURRENT
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
    //恢复versionset
    error = recoverVersions(db->versions, save_manifest);
    if (!isOk(error)) {
        return error;
    }
    printf("aaaaan\n");
    SequenceNumber max_sequence = 0;
    // 从所有比描述符中命名的日志文件更新的日志文件中恢复
    //（前一个版本可能添加了新的日志文件，但没有在描述符中注册它们）。
    //
    // 请注意，PrevLogNumber() 不再使用，但我们要注意它，
    //以防我们恢复由旧版本的 leveldb 生成的数据库。
    // const uint64_t min_log = db->versions->log_number;
    // const uint64_t prev_log = db->versions->prev_log_number;
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