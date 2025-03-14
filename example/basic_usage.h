//
// Created on 2025/3/14.
//

#ifndef BASIC_USAGE_H
#define BASIC_USAGE_H

#include <iostream>

#include "simple_refl.h"

namespace basic_usage {
    /**
     * Here is a class for demonstrating the usage of simple_refl.
     * @tparam T Type of the member.
     */
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    class Vector3 {
    public:
        T x, y, z;

        Vector3() : x(0), y(0), z(0) {
        }

        Vector3(T x, T y, T z) : x(x), y(y), z(z) {
        }

        std::tuple<T, T, T> fetch_add(T x, T y, T z) {
            return std::make_tuple(this->x += x, this->y += y, this->z += z);
        }

        Vector3 operator+(T scalar) {
            return Vector3(x + scalar, y + scalar, z + scalar);
        }

        Vector3 operator+(const Vector3& other) {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        Vector3 operator*(T scalar) const {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }
    };

    // create a reflection object to register the members and methods of Vector3.
    static simple_reflection::ReflectionBase reflection = simple_reflection::make_reflection()
            // to register the 3 members of Vector3.
            .register_member<&Vector3<float>::x>("x")
            .register_member<&Vector3<float>::y>("y")
            .register_member<&Vector3<float>::z>("z")
            .register_method<&Vector3<float>::fetch_add>("fetch_add")
            // to register the 2 methods of Vector3 which has no overload.
            .register_method<&Vector3<float>::operator*>("operator*")
            // to register the overloaded method of Vector3.
            .register_method<Vector3<float>, Vector3<float>, float>("operator+", &Vector3<float>::operator+)
            .register_method<Vector3<float>, Vector3<float>, const Vector3<float>&>(
                "operator+", &Vector3<float>::operator+)
            // to register the constructors of Vector3.
            .register_function<Vector3<float>, float, float, float>(
                "ctor",
                [](float x, float y, float z) {
                    return Vector3<float>(x, y, z);
                })
            .register_function<Vector3<float>>("ctor", []() {
                return Vector3<float>();
            });

    inline void demonstrate() {
        // invoke the constructor of Vector3
        auto vec = reflection.invoke_function<Vector3<float>>("ctor", 1.0f, 2.0f, 3.0f);
        void* ptr = &vec; // get the pointer to the object, type-erased.

        std::cout << "vec.x before: " << vec.x << std::endl;
        auto* x = reflection.get_member_ref<float>(ptr, "x"); // use pointer to access the member.
        *x = 10.0;
        std::cout << "vec.x after: " << vec.x << std::endl;

        std::cout << "vec.y before: " << vec.y << std::endl;
        auto* y = reflection.get_member_ref<float>(vec, "y"); // ...while using reference to access the member is ok.
        *y = 20.0;
        std::cout << "vec.y after: " << vec.y << std::endl;

        std::cout << "vec.z before: " << vec.z << std::endl;
        auto* z = reflection.get_member_ref<float>(ptr, "z");
        *z = 30.0;
        std::cout << "vec.z after: " << vec.z << std::endl;

        // invoke the method of Vector3, which takes 3 parameters, and returns a tuple.
        auto ret = reflection.invoke_method<std::tuple<float, float, float>>(vec, "fetch_add", 1.0f, 2.0f, 3.0f);
        std::cout << "vec.x after fetch_add: " << std::get<0>(ret) << std::endl;
        std::cout << "vec.y after fetch_add: " << std::get<1>(ret) << std::endl;
        std::cout << "vec.z after fetch_add: " << std::get<2>(ret) << std::endl;

        // invoke the method of Vector3, which takes 1 parameter, and returns another Vector3.
        auto ret2 = reflection.invoke_method<Vector3<float>, Vector3<float>>(vec, "operator*", 2.0f);
        std::cout << "result of operator*: " << ret2.x << std::endl;
        std::cout << "result of operator*: " << ret2.y << std::endl;
        std::cout << "result of operator*: " << ret2.z << std::endl;

        // invoke the method of Vector3, which takes 2 parameters, and returns another Vector3.
        // and this method has two overloads.
        // use the overload which takes a Vector3 as the only parameter.
        auto vec2 = Vector3<float>(1.0f, 2.0f, 3.0f);
        auto ret3 = reflection.invoke_method<Vector3<float>, Vector3<float>>(vec, "operator+", vec2);
        std::cout << "result of operator+: " << ret3.x << std::endl;
        std::cout << "result of operator+: " << ret3.y << std::endl;
        std::cout << "result of operator+: " << ret3.z << std::endl;
    }
} // basic_usage

#endif //BASIC_USAGE_H
