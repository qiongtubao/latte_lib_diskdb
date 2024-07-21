#ifndef __LATTE_DISKDB_VERSION_BUILDER_H
#define __LATTE_DISKDB_VERSION_BUILDER_H

#include "versionSet.h"
#include "version.h"


typedef struct LevelState {
    set* deleted_files; //set<uint64_t>
    set* added_files;   //set<FileMetaData*> 顺序存储 
} LevelState;
typedef struct VersionSetBuilder {
    VersionSet* vset;
    Version* base;
    LevelState levels[7]; //kNumLevels
} VersionSetBuilder;
VersionSetBuilder* versionSetBuilderCreate();

void versionSetBuilderApply(VersionSetBuilder* builer, VersionEdit* edit);
#endif