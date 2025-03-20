//
// Created on 2025/3/20.
//

#ifndef DERIVE_TEST_H
#define DERIVE_TEST_H

#include <iostream>

#include "simple_refl.h"
#include "test_helper.h"

namespace derive_test {
    class Base {
    public:
        int x;

        int get_x() const {
            return x;
        }

        Base() {
            x = 0;
        }
    };

    static auto base_refl = simple_reflection::make_reflection<Base>()
        .register_method<&Base::get_x>("get_x")
        .register_member<&Base::x>("x")
        .register_function<Base>("ctor", []() { return Base(); });

    class Derived : public Base {
    public:
        int y;

        int get_y() const {
            return y;
        }

        Derived() {
            y = 0;
        }
    };

    static auto derived_refl = simple_reflection::make_reflection<Derived>()
        .derives_from<Base>()
        .register_method<&Derived::get_y>("get_y")
        .register_member<&Derived::y>("y")
        .register_function<Derived>("ctor", []() { return Derived(); });

    inline void base_derive_test() {
        auto derived = derived_refl.invoke_function("ctor");
        std::cout << "derived.get_x() = " << *derived_refl.get_member_ref<int>(derived.get_raw(), "x") << std::endl;
    }

    inline void run_tests() {
        begin_test("derive_test") {
            test(base_derive_test)
        } end_test()
    }
}

#endif //DERIVE_TEST_H
