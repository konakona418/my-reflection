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

    void run_tests() {
        begin_test("helpers") {
            test(helper_tests::test_can_cast_to);
            // test(helper_tests::test_function_cast);
        } end_test()
    }
}

#endif //HELPER_TESTS_H
