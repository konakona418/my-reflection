//
// Created on 2025/3/14.
//

#ifndef HELPER_TESTS_H
#define HELPER_TESTS_H

#include <any>
#include <iostream>

#include "test_helper.h"
#include "simple_refl.h"

namespace helper_tests {
    void test_can_cast_to() {
        std::any x = static_cast<float>(3.14);
        assert(simple_reflection::can_cast_to<float>(x));
        assert(!simple_reflection::can_cast_to<int>(x));
    }

    void test_function_cast() {
        class Vector3 {
            float x, y, z;

        public:
            Vector3(float x, float y, float z) : x(x), y(y), z(z) {
            }

            float get_x() {
                return x;
            }
        };
        auto fn = std::function<float(Vector3*)>(&Vector3::get_x);
        assert(simple_reflection::can_cast_to<float(void*)>(fn));
    }

    class TestClass {
    public:
        float x, y;
        TestClass(float x, float y) : x(x), y(y) {
        }
        float test_function(float x, float y) {
            this->x = x;
            this->y = y;
            return x + y;
        }
        void test_void_function(float x, float y) {
            this->x = x;
            this->y = y;
        }
    };

    void any_wrapper_test() {
        TestClass test_class(0.0f, 0.0f);
        auto x = simple_reflection::wrap_method(&TestClass::test_function);
        float a, b;
        a = 3.2f;
        b = 2.1f;
        void* pack[] = refl_args(a, b);
        auto invoke_result = (x(&test_class, pack));

        auto y = simple_reflection::wrap_method(&TestClass::test_void_function);
        y(&test_class, pack);
        std::cout << invoke_result.get<float>() << std::endl;
        assert(invoke_result.get<float>() == 5.3f);
        assert(test_class.x == 3.2f);
        assert(test_class.y == 2.1f);
    }

    void run_tests() {
        begin_test("helpers") {
            test(helper_tests::test_can_cast_to);
            // test(helper_tests::test_function_cast);
            test(helper_tests::any_wrapper_test);
        } end_test()
    }
}

#endif //HELPER_TESTS_H
