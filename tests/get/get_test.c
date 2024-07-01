
#include <test/testhelp.h>
#include <test/testassert.h>

int test_get() {
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