//
// Created on 2025/3/14.
//

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <cassert>

namespace test_helper {
#define begin_test(test_name) \
    std::cout << "begin test: " << test_name << std::endl; \
    do {\
        int _test_count = 0; \
        int _test_passed = 0; \
        do

#define test(fn) \
        _test_count++;\
        try { \
        std::cout << "testing: " << #fn << std::endl; \
        fn(); \
        std::cout << "test passed" << std::endl << std::endl;\
        _test_passed++;\
        } catch (const std::exception& e) { \
        std::cout << "test failed: " << e.what() << std::endl << std::endl; \
        }

#define end_test() \
        while (false); \
        std::cout << "test count: " << _test_count << std::endl; \
        std::cout << "test passed: " << _test_passed << std::endl; \
        std::cout << "test failed: " << _test_count - _test_passed << std::endl << std::endl; \
        assert(_test_passed == _test_count);\
    } while (false);
}

#endif //TEST_HELPER_H
