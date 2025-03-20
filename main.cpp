#define EXAMPLE
#define TEST



#ifdef TEST
// #include "test/basic_tests.h"
// #include "test/helper_tests.h"
// #include "test/generic_tests.h"
// #include "test/function_tests.h"
#include "test/derive_test.h"
#endif

#ifdef EXAMPLE
// #include "example/basic_usage.h"
// #include "example/json_parser.h"
#endif

int main() {
#ifdef TEST
    // helper_tests::run_tests();
    // basic_tests::run_tests();
    // generic_tests::run_tests();
    // function_tests::run_tests();
    derive_test::run_tests();
#endif
#ifdef EXAMPLE
    // basic_usage::demonstrate();
    // basic_usage::demonstrate_type_erasure();
    // json_parser_test::test_parse_json();
#endif
    return 0;
}
