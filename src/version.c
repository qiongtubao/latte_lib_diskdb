#include "version.h"
//
Version* versionCreate(VersionSet* vset) {
    Version* version = zmalloc(sizeof(Version));
    version->vset = vset;
    version->next = version;
    version->prev = version;
    version->refs = 0;
    version->file_to_compact = NULL;
    version->file_to_compact_level = -1;
    version->compaction_score = -1;
    version->compaction_level = -1;
    return version;
}


void versionRelease(Version* version) {
    version->prev->next = version->next;
    version->next->prev = version->prev;

    for (int level = 0; level < 7; level++) {
        list* vfiles = version->files[level];
        listNode* node;
        while ((node = listNext(vfiles)) != NULL) {
            FileMetaData* f = listNodeValue(node);
            assert(f->refs > 0);
            f->refs--;
            if (f->refs <= 0) {
                fileMetaDataRelease(f);
            }
        }
        
    }

}

void versionUnref(Version* version) {
    version->refs--;
    if (version->refs == 0) {
        versionRelease(version);
    }
}

void versionRef(Version* version) {
    version->refs++;
}
