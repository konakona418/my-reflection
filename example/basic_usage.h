//
// Created on 2025/3/14.
//

#ifndef BASIC_USAGE_H
#define BASIC_USAGE_H

#include <iostream>

#include "simple_refl.h"

namespace basic_usage {

    class Vec3Internal {
        int a = 0;
    public:
        int r, g, b;

        Vec3Internal(int r, int g, int b) : r(r), g(g), b(b) {
        }
    };
    /**
     * Here is a class for demonstrating the usage of simple_refl.
     * @tparam T Type of the member.
     */
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    class Vector3 {
    public:
        T x, y, z;
        Vec3Internal internal = {0, 0, 0};

        Vector3() : x(0), y(0), z(0) {
        }

        Vector3(T x, T y, T z) : x(x), y(y), z(z) {
        }

        std::tuple<T, T, T> fetch_add(T x, T y, T z) {
            return std::make_tuple(this->x += x, this->y += y, this->z += z);
        }

        std::tuple<T, T, T> fetch_sub(T x, T y, T z) {
            return std::make_tuple(this->x -= x, this->y -= y, this->z -= z);
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

        float sum_three(T x, T y, T z) {
            this->x = x;
            this->y = y;
            this->z = z;
            return x + y + z;
        }

        float sum_mul(T x, int y, T z, T w) {
            return x * y * z * w;
        }
    };

    // create a reflection object to register the members and methods of Vector3.
    static auto& reflection = simple_reflection::make_reflection<Vector3<float>>()
            // to register the members of Vector3.
            .register_member<&Vector3<float>::x>("x")
            .register_member<&Vector3<float>::y>("y")
            .register_member<&Vector3<float>::z>("z")
            .register_member<&Vector3<float>::internal>("internal")
            // to register the methods of Vector3.
            .register_method<&Vector3<float>::fetch_add>("fetch_add")
            .register_method<&Vector3<float>::fetch_sub>("fetch_sub")
            .register_method<&Vector3<float>::sum_three>("sum_three")
            .register_method<&Vector3<float>::sum_mul>("sum_mul")
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

    inline void demonstrate_type_erasure() {
        std::cout << "demonstrate type erasure:" << std::endl;

        // create a helper object to hold the phantom data,
        // this is because we want to reuse the same identifier for different return value.
        // if we don't do so, the data can be invalidated.
        simple_reflection::PhantomDataHelper phantom;

        // here we use a helper function to convert the arguments to a simple_reflection::ArgList
        auto args = make_args(1.0f, 2.0f, 3.0f);

        // then invoke the constructor of Vector3...
        auto proxy = reflection.invoke_function("ctor", args);
        // ...which returns a simple_reflection::ReturnValueProxy.
        // this object manages the life cycle of the return value, so there's no need to worry about memory leak.
        // at least I guess so.

        // we can preserve the type information of the class here (as we just invoked the constructor),
        // in case we want to use it later.
        auto type_info = proxy.get_type_index();

        // get the pointer to the object, type-erased.
        void* ptr = proxy.get_raw();

        // get the wrapped objects of the members respectively.
        // this is a type which wraps the pointer along with the type information.
        auto x_wrapped = reflection.get_member_wrapped(ptr, "x");
        auto y_wrapped = reflection.get_member_wrapped(ptr, "y");
        auto z_wrapped = reflection.get_member_wrapped(ptr, "z");

        std::cout << "vec.x: " << x_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.y: " << y_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.z: " << z_wrapped.deref_into<float>() << std::endl;

        // can also set the value of the wrapped objects.
        x_wrapped.set_value(10.0f);
        std::cout << "vec.x after set: " << x_wrapped.deref_into<float>() << std::endl;

        // convert the wrapped objects to a simple_reflection::RawObjectWrapperVec.
        simple_reflection::RawObjectWrapperVec vec;
        vec.push_back(x_wrapped);

        // we can then convert the wrapped objects to a simple_reflection::ArgList
        // and you can actually merge arguments like this.
        auto args2 = refl_arg_list(vec) | y_wrapped | z_wrapped;

        // we are about to rewrite the return value, so we need to duplicate the original one,
        // in order to avoid the original one being invalidated by the auto memory de-allocation of shared_ptr<void>.
        phantom.push(proxy.duplicate_inner());
        // invoke the method of Vector3, which takes 3 parameters, and returns a tuple.
        proxy = reflection.invoke_method(ptr, "fetch_add", args2);
        std::cout << "vec.x after fetch_add: " << x_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.y after fetch_add: " << y_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.z after fetch_add: " << z_wrapped.deref_into<float>() << std::endl;

        // and we can utilize the proxy to get the return value (the tuple) as well.
        auto result = proxy.get<std::tuple<float, float, float>>();
        std::cout << "result of fetch_add: "
                << std::get<0>(result) << " "
                << std::get<1>(result) << " "
                << std::get<2>(result) << std::endl;

        // similar to the previous case, we need to duplicate the original one.
        // this is a simplified way to do so.
        phantom << proxy;

        // and initializer list is also supported.
        proxy = reflection.invoke_method(ptr, "fetch_sub",
            merge_arg_list(
                // and merging arg lists is also supported.
                simple_reflection::ArgList {x_wrapped},
                simple_reflection::refl_args(2.0f))
                | simple_reflection::refl_args(1.0f));

        std::cout << "vec.x after fetch_sub: " << x_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.y after fetch_sub: " << y_wrapped.deref_into<float>() << std::endl;
        std::cout << "vec.z after fetch_sub: " << z_wrapped.deref_into<float>() << std::endl;

        result = proxy.get<std::tuple<float, float, float>>();
        std::cout << "result of fetch_sub: "
                << std::get<0>(result) << " "
                << std::get<1>(result) << " "
                << std::get<2>(result) << std::endl;

        // now we create a new Vector3 and invoke the method of Vector3.
        auto vec2_args = simple_reflection::refl_args(4.0f, 5.0f, 6.0f);
        auto vec2_proxy = reflection.invoke_function("ctor", vec2_args);
        void* ptr2 = vec2_proxy.get_raw();

        // we then build a simple_reflection::ArgList from the wrapped object.
        auto add_args = simple_reflection::RawObjectWrapperVec{
            // note that we cannot use refl_args here,
            // for void* does not contain any valid type information for checks during method invocation.
            // we can use the type information we preserved earlier,
            // but we can also fetch it from ReflectionBase, like this:
            simple_reflection::wrap_object(ptr2, reflection.get_class_type())
        };
        auto add_args_parsed = refl_arg_list(add_args);

        // also ok
        proxy >> phantom;

        // invoke the method of Vector3, which takes 1 parameter, and returns another Vector3.
        proxy = reflection.invoke_method(ptr, "operator+", add_args_parsed);

        /** this has the same effect with the previous invocation,
         as ReturnValueProxy::to_wrapped() preserves the type information:
        proxy = reflection.invoke_method(ptr, "operator+",
            simple_reflection::empty_arg_list() | vec2_proxy.to_wrapped());*/
        auto ptr_vec3 = proxy.get_raw();

        // get the members of the returned object.
        x_wrapped = reflection.get_member_wrapped(ptr_vec3, "x");
        y_wrapped = reflection.get_member_wrapped(ptr_vec3, "y");
        z_wrapped = reflection.get_member_wrapped(ptr_vec3, "z");

        // print the result
        std::cout << "result of operator+: " << x_wrapped.deref_into<float>() << std::endl;
        std::cout << "result of operator+: " << y_wrapped.deref_into<float>() << std::endl;
        std::cout << "result of operator+: " << z_wrapped.deref_into<float>() << std::endl;

        proxy >> phantom;

        // and you can use comma separated arguments to invoke the method of Vector3,
        // which takes 4 parameters, and returns a tuple.
        // the way comma works is completely the same with the pipe operator.
        proxy = reflection.invoke_method(ptr, "sum_mul",
            (simple_reflection::empty_arg_list(), x_wrapped, make_args(1), y_wrapped, z_wrapped));

        std::cout << "result of sum_mul: " << proxy.get<float>() << std::endl;

        reflection.set_member(ptr, "internal", simple_reflection::wrap_object(Vec3Internal{1, 2, 3}));
        auto internal = reflection.get_member_wrapped(ptr, "internal");
        std::cout << "internal.r: " << internal.deref_into<Vec3Internal>().r << std::endl;
        std::cout << "internal.g: " << internal.deref_into<Vec3Internal>().g << std::endl;
        std::cout << "internal.b: " << internal.deref_into<Vec3Internal>().b << std::endl;
    }
} // basic_usage

#endif //BASIC_USAGE_H
