
#include <test/testhelp.h>
#include <test/testassert.h>
#include <sds/sds.h>
#include <status.h>
#include <db.h>
#include <read.h>
#include <write.h>

int test_get() {
    DiskDb* db;
    DiskDbOptions op;
    DiskDbReadOptions rp;
    DiskDbWriteOptions wp;
    op.create_if_missing = true;
    DiskDbState status = diskDbOpen(op, "./testdb", &db);
    sds result ;
    status = diskDbGet(db, rp, "hello", &result);
    assert(NotFound == status);
    status = diskDbPut(db,wp, "hello", "world");
    assert(Ok == status);
    status = diskDbGet(db, rp, "hello", &result);
    assert(Ok == status);
    assert(strcmp(result, "world") == 0);
    return 1;
}
int test_api(void) {
    {
        test_cond("get function",
            test_get() == 1);
    } test_report()
    return 1;
}

int main() {
    test_api();
    return 0;
}