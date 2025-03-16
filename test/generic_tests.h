//
// Created on 2025/3/14.
//

#ifndef GENERIC_TESTS_H
#define GENERIC_TESTS_H

#include <cmath>
#include <cassert>
#include <iostream>
#include <string>

#include "test_helper.h"
#include "simple_refl.h"

namespace generic_tests {
    template <typename T>
    struct Vector3 {
        T x;
        T y;
        T z;

        Vector3(T x, T y, T z) : x(x), y(y), z(z) {
        }

        Vector3() : x(0), y(0), z(0) {
        }

        T len() const {
            return std::sqrt(x * x + y * y + z * z);
        }
    };

    static auto refl = simple_reflection::make_reflection<Vector3<float>>()
            .register_member<&Vector3<float>::x>("x")
            .register_member<&Vector3<float>::y>("y")
            .register_member<&Vector3<float>::z>("z")
            .register_method<&Vector3<float>::len>("len");

    void test_basic_register() {
        auto vec = Vector3<float>(1.0f, 2.0f, 3.0f);

        assert(refl.get_member_ref<float>(vec, "x") != nullptr &&
            *refl.get_member_ref<float>(vec, "x") == 1.0f);
        assert(refl.get_member_ref<float>(vec, "y") != nullptr &&
            *refl.get_member_ref<float>(vec, "y") == 2.0f);
        assert(refl.get_member_ref<float>(vec, "z") != nullptr &&
            *refl.get_member_ref<float>(vec, "z") == 3.0f);

        std::cout << *refl.get_member_ref<float>(vec, "x") << std::endl;
        std::cout << *refl.get_member_ref<float>(vec, "y") << std::endl;
        std::cout << *refl.get_member_ref<float>(vec, "z") << std::endl;

        *refl.get_member_ref<float>(vec, "z") = 10.0f;

        assert(*refl.get_member_ref<float>(vec, "z") == 10.0f);
        std::cout << *refl.get_member_ref<float>(vec, "z") << std::endl;
    }

    void test_invocation() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);
        assert(std::round(refl.invoke_method<float>(vec, "len")) == 4.0f);
    }

    void run_tests() {
        begin_test("generic") {
            test(generic_tests::test_basic_register);
            test(generic_tests::test_invocation);
        } end_test()
    }
}

#endif //GENERIC_TESTS_H
