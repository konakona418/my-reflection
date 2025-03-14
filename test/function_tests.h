//
// Created on 2025/3/14.
//

#ifndef FUNCTION_TEST_H
#define FUNCTION_TEST_H

#include <iostream>

#include "simple_refl.h"
#include "test_helper.h"

namespace function_tests {
    class Vector3 {
    public:
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {
        }

        Vector3(float x, float y, float z) : x(x), y(y), z(z) {
        }

        friend Vector3 operator+(const Vector3& lhs, const Vector3& rhs) {
            return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
        }
    };

    static auto refl = simple_reflection::make_reflection()
            .register_member<&Vector3::x>("x")
            .register_member<&Vector3::y>("y")
            .register_member<&Vector3::z>("z")
            .register_function<Vector3, const Vector3&, const Vector3&>("operator+",
                                                                        [](const Vector3& lhs, const Vector3& rhs) {
                                                                            return lhs + rhs;
                                                                        })
            .register_function<Vector3, float, float, float>("ctor", [](float x, float y, float z) {
                return Vector3(x, y, z);
            })
            .register_function<Vector3>("ctor", []() {
                return Vector3();
            });

    inline void test_operator_sum() {
        Vector3 v1(1, 2, 3), v2(4, 5, 6);
        const auto result = refl.invoke_function<Vector3>("operator+", v1, v2);
        assert(result.x == 5 && result.y == 7 && result.z == 9);
    }

    inline void test_ctor() {
        const auto result = refl.invoke_function<Vector3>("ctor", 1.0f, 2.0f, 3.0f);
        assert(result.x == 1 && result.y == 2 && result.z == 3);
    }

    inline void test_default_ctor() {
        const auto result = refl.invoke_function<Vector3>("ctor");
        assert(result.x == 0 && result.y == 0 && result.z == 0);
    }

    inline void run_tests() {
        begin_test("function") {
            test(test_operator_sum);
            test(test_ctor);
            test(test_default_ctor);
        } end_test()
    }
}

#endif //FUNCTION_TEST_H
