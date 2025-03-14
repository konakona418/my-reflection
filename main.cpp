#include <cassert>
#include <iostream>
#include <cmath>

#include "simple_refl.h"

#define begin_test(test_name) \
    std::cout << "begin test: " << test_name << std::endl; \
    do {\
    int _test_count = 0; \
    int _test_passed = 0;

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
    std::cout << "test count: " << _test_count << std::endl; \
    std::cout << "test passed: " << _test_passed << std::endl; \
    std::cout << "test failed: " << _test_count - _test_passed << std::endl << std::endl; \
    assert(_test_passed == _test_count);\
    } while (false);

namespace helper_tests {
    void test_can_cast_to() {
        std::any x = static_cast<float>(3.14);
        assert(simple_reflection::can_cast_to<float>(x));
        assert(!simple_reflection::can_cast_to<int>(x));
    }
}

namespace tests {
    class Vector3 {
        int _placeholder[4];
        char _placeholder2[3];

    public:
        const int k_placeholder = 114514;
        float x, y, z;
        int array[3] = {5, 6, 7};

        Vector3() : x(0), y(0), z(0) {
        }

        Vector3(float x, float y, float z) : x(x), y(y), z(z) {
        }

        friend Vector3 operator+(const Vector3& lhs, const Vector3& rhs) {
            return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
        }

        float len() const {
            return std::sqrt(std::pow(x, 2.0f) + std::pow(y, 2.0f) + std::pow(z, 2.0f));
        }

        void add_x(float x) {
            this->x += x;
        }

        float fetch_add_x(float x) {
            float ret = x;
            this->x += x;
            return ret;
        }

        void add_x_by_1() {
            x += 1;
        }

        int overload_add_x(int x) {
            return (this->x += x, x);
        }

        float overload_add_x(float x) {
            return (this->x += x, this->x);
        }

        std::tuple<float, float> fetch_add_x_and_y(float x, float y) {
            float ret_x = x;
            float ret_y = y;
            this->x += x;
            this->y += y;
            return std::make_tuple(ret_x, ret_y);
        }
    };

    static auto refl = simple_reflection::make_reflection()
            .register_member<&Vector3::x>("x")
            .register_member<&Vector3::y>("y")
            .register_member<&Vector3::z>("z")
            .register_member<&Vector3::array>("array")
            .register_member<&Vector3::k_placeholder>("k_placeholder")
            .register_method<&Vector3::len>("len")
            .register_method<&Vector3::add_x>("add_x")
            .register_method<&Vector3::fetch_add_x>("fetch_add_x")
            .register_method<&Vector3::add_x_by_1>("add_x_by_1")
            .register_method<&Vector3::fetch_add_x_and_y>("fetch_add_x_and_y")
            .register_method<Vector3, float, float>("overload_add_x", &Vector3::overload_add_x)
            .register_method<Vector3, int, int>("overload_add_x", &Vector3::overload_add_x);



    void test_basic_register() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        auto* p_x = refl.get_member_ref<float>(vec, "x");
        assert(p_x != nullptr && *p_x == 1.0f);

        auto* p_y = refl.get_member_ref<float>(vec, "y");
        assert(p_y != nullptr && *p_y == 2.0f);

        std::cout << *p_x << std::endl;
        std::cout << *p_y << std::endl;

        auto* p_z = refl.get_member_ref<float>(vec, "z");
        assert(p_z != nullptr && *p_z == 3.0f);

        *p_z = 10.0f;
        assert(p_z != nullptr && *p_z == 10.0f);
        std::cout << *p_z << std::endl;

        const auto array = *refl.get_member_ref<int[3]>(vec, "array");
        assert(array[0] == 5 && array[1] == 6 && array[2] == 7);
        std::cout << array[0] << " " << array[1] << " " << array[2] << std::endl;
    }

    void test_const_register() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        const auto k_placeholder = refl.get_const_member_ref<int>(vec, "k_placeholder");
        assert(k_placeholder != nullptr && *k_placeholder == 114514);
        std::cout << *k_placeholder << std::endl;
    }

    void test_incorrect_member_type() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        assert(refl.get_member_ref<float>(vec, "x") != nullptr);
        assert(refl.get_member_ref<int>(vec, "y") == nullptr);
    }

    void test_member_is_const() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        assert(refl.is_member_const<int>(vec, "k_placeholder"));
        assert(refl.is_member_const<const int>(vec, "k_placeholder"));
        assert(!refl.is_member_const<char>(vec, "k_placeholder"));

        assert(!refl.is_member_const<float>(vec, "x"));
        assert(!refl.is_member_const<const float>(vec, "x"));
        assert(!refl.is_member_const<int>(vec, "x"));
    }

    void test_default_ctor_invocation() {
        auto vec = refl.invoke_ctor<Vector3>();
        assert(vec.x == 0.0f && vec.y == 0.0f && vec.z == 0.0f);
        std::cout << vec.x << " " << vec.y << " " << vec.z << std::endl;
    }

    void test_basic_ctor_invocation() {
        auto vec2 = refl.invoke_ctor<Vector3>(1.0f, 2.0f, 3.0f);
        assert(vec2.x == 1.0f && vec2.y == 2.0f && vec2.z == 3.0f);
        std::cout << vec2.x << " " << vec2.y << " " << vec2.z << std::endl;
    }

    void test_method_no_ret_no_param() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        refl.invoke_method<void>(vec, "add_x_by_1");
        std::cout << vec.x << std::endl;
        assert(vec.x == 2.0f);
    }

    void test_method_no_ret_has_param() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);
        refl.invoke_method(vec, "add_x", 1.0f);
        std::cout << vec.x << std::endl;
        assert(vec.x == 2.0f);
    }

    void test_method_has_ret_no_param() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        auto len = refl.invoke_const_method<float>(vec, "len");
        std::cout << len << std::endl;
        assert(std::round(len) == 4);
    }

    void test_method_has_ret_has_param() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);
        auto ret = refl.invoke_method<float>(vec, "fetch_add_x", 1.0f);
        std::cout << ret << std::endl;

        assert(ret == 1.0f && vec.x == 2.0f);
    }

    void test_method_has_ret_tuple_has_multiple_param() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);
        auto [x, y] = refl.invoke_method<std::tuple<float, float>>(vec, "fetch_add_x_and_y", 1.0f, 2.0f);
        std::cout << x << " " << y << std::endl;

        assert(x == 1.0f && y == 2.0f);
        assert(vec.x == 2.0f && vec.y == 4.0f);
    }

    void test_const_method() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);
        assert(std::round(refl.invoke_const_method<float>(vec, "len")) == 4.0f);
    }

    void test_method_is_const() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        assert(refl.is_method_const(vec, "len"));
        assert(!refl.is_method_const(vec, "add_x_by_1"));
    }

    void test_method_overload() {
        auto vec = Vector3(1.0f, 2.0f, 3.0f);

        assert(refl.invoke_method<float>(vec, "overload_add_x", 2.0f) == 3.0f);
        assert(refl.invoke_method<int>(vec, "overload_add_x", 1) == 1);
    }
}

namespace generic_tests {
    template <typename T>
    struct Vector3 {
        T x;
        T y;
        T z;

        Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

        Vector3() : x(0), y(0), z(0) {}

        T len() const {
            return std::sqrt(x * x + y * y + z * z);
        }
    };

    static auto refl = simple_reflection::make_reflection()
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
        assert(std::round(refl.invoke_const_method<float>(vec, "len")) == 4.0f);
    }
}

int main() {
    begin_test("helpers") {
        test(helper_tests::test_can_cast_to);
    } end_test()

    begin_test("basic") {
        test(tests::test_basic_register);
        test(tests::test_const_register);
        test(tests::test_incorrect_member_type);
        test(tests::test_member_is_const);

        test(tests::test_default_ctor_invocation);
        test(tests::test_basic_ctor_invocation);

        test(tests::test_method_has_ret_no_param);
        test(tests::test_method_has_ret_has_param);
        test(tests::test_method_no_ret_no_param);
        test(tests::test_method_no_ret_has_param);
        test(tests::test_method_has_ret_tuple_has_multiple_param);
        test(tests::test_const_method);
        test(tests::test_method_is_const);
        test(tests::test_method_overload);
    } end_test();

    begin_test("generic") {
        test(generic_tests::test_basic_register);
        test(generic_tests::test_invocation);
    } end_test()
}
