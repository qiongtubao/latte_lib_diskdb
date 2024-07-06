
#include <test/testhelp.h>
#include <test/testassert.h>
#include <sds/sds.h>
#include <status.h>
#include <db.h>
#include <read.h>
#include <write.h>
#include <utils/error.h>
int test_get() {
    DiskDb* db;
    DiskDbOptions op;
    DiskDbReadOptions rp;
    DiskDbWriteOptions wp;
    op.create_if_missing = true;
    Error* status = diskDbOpen(op, "./testdb", &db);
    sds result ;
    status = diskDbGet(db, rp, "hello", &result);
    assert(CNotFound == status->code);
    status = diskDbPut(db, wp, "hello", "world");
    printf("\n%d\n", status->code);
    assert(isOk(status));
    status = diskDbGet(db, rp, "hello", &result);
    assert(isOk(status));
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