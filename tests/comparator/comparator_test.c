
#include <test/testhelp.h>
#include <test/testassert.h>
#include <sds/sds.h>
#include <status.h>
#include <db.h>
#include <read.h>
#include <write.h>
#include <utils/error.h>
#include "comparator.h"


int test_byte_comparator_compare() {
    sds a = sdsnew("hello");
    sds b = sdsnew("hello");
    assert(0 == bytewiseComparator.compare(a, b));
    sdsfree(a);
    sdsfree(b);

    a = sdsnew("hello");
    b = sdsnew("hella");
    assert(0 < bytewiseComparator.compare(a, b));
    sdsfree(a);
    sdsfree(b);

    a = sdsnew("hello");
    b = sdsnew("hello world");
    assert(0 > bytewiseComparator.compare(a, b));
    assert(0 < bytewiseComparator.compare(b, a));
    sdsfree(a);
    sdsfree(b);

    a = sdsnew("");
    b = sdsnew("");
    assert(0 == bytewiseComparator.compare(a, b));
    sdsfree(a);
    sdsfree(b);
    return 1;
}

int test_byte_comparator_findShortestSeparator() {
    sds a = sdsnew("abcd");
    sds limit = sdsnew("abec");
    bytewiseComparator.findShortestSeparator(&a, limit);
    sds result = sdsnew("abd");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);

    a = sdsnew("abcd");
    limit = sdsnew("abcdef");
    bytewiseComparator.findShortestSeparator(&a, limit);
    result = sdsnew("abcd");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);

    a = sdsnew("abcd");
    limit = sdsnew("abcd");
    bytewiseComparator.findShortestSeparator(&a, limit);
    result = sdsnew("abcd");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);

    a = sdsnew("xyz");
    limit = sdsnew("abcd");
    bytewiseComparator.findShortestSeparator(&a, limit);
    result = sdsnew("xyz");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);

    a = sdsnew("abc\x7F");
    limit = sdsnew("abc\x7F\x00");
    bytewiseComparator.findShortestSeparator(&a, limit);
    result = sdsnew("abc\x7F");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);

    a = sdsnew("a");
    limit = sdsnew("cdefghijklmnopqrstuvwxyz");
    bytewiseComparator.findShortestSeparator(&a, limit);
    result = sdsnew("b");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(limit);
    sdsfree(result);
    return 1;
}

int test_byte_comparator_findShortSuccessor() {
    sds a = sdsnew("abcd");
    bytewiseComparator.findShortSuccessor(&a);
    sds result = sdsnew("b");
    assert(0 == sdscmp(a, result));
    sdsfree(a);
    sdsfree(result);
    return 1;
}
int test_api(void) {
    {
        test_cond("byte comparator compare function",
            test_byte_comparator_compare() == 1);
        test_cond("byte comparator findShortestSeparator function",
            test_byte_comparator_findShortestSeparator() == 1);
        test_cond("byte comparator findShortSuccessor function",
            test_byte_comparator_findShortSuccessor() == 1);
    } test_report()
    return 1;
}

int main() {
    test_api();
    return 0;
}