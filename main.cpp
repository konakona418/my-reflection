#define TEST

#ifdef TEST
#include "test/basic_tests.h"
#include "test/helper_tests.h"
#include "test/generic_tests.h"
#include "test/function_tests.h"
#endif

int main() {
#ifdef TEST
    helper_tests::run_tests();
    basic_tests::run_tests();
    generic_tests::run_tests();
    function_tests::run_tests();
#endif
}
