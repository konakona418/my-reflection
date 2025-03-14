// Simple Reflection Library.
// Created on 2025/3/13.
//
// This library is a simple reflection library designed for C++.
// With it, you can reflect the members and methods of a class, during runtime.

#ifndef SIMPLE_REFL_H
#define SIMPLE_REFL_H

#include <string>
#include <unordered_map>
#include <any>
#include <tuple>
#include <exception>
#include <utility>
#include <vector>
#include <functional>

/** Simple Reflection Library. */
namespace simple_reflection {
    template <typename FullName>
    struct extract_member_type;

    /**
     * Extract the member type and parent type from a full name.
     * @tparam ClassType The type of the class.
     * @tparam MemberType The type of the member.
     */
    template <typename ClassType, typename MemberType>
    struct extract_member_type<MemberType ClassType::*> {
        using type = MemberType;
        using parent_type = ClassType;
    };

    /**
     * Extract the member type and parent type from a full name.
     * @tparam FullName The full name of the type of the member.
     */
    template <typename FullName>
    using extract_member_type_t = typename extract_member_type<FullName>::type;

    /**
     * Extract the parent type from a full name.
     * @tparam FullName The full name of the type of the member.
     */
    template <typename FullName>
    using extract_member_parent_t = typename extract_member_type<FullName>::parent_type;

    /**
     * Remove const prefix from a type.
     * @tparam TypeName The type to remove const suffix from.
     */
    template <typename TypeName>
    struct remove_const {
        using type = TypeName;
    };

    /**
     * Remove const prefix from a type.
     * @tparam TypeName The type to remove const suffix from.
     */
    template <typename TypeName>
    struct remove_const<const TypeName> {
        using type = TypeName;
    };

    /**
     * Remove const suffix from a type.
     * @tparam TypeName The type to remove const suffix from.
     */
    template <typename TypeName>
    struct remove_const_suffix {
        using type = TypeName;
    };

    /**
     * Remove const suffix from a type.
     * @tparam TypeName The type to remove const suffix from.
     */
    template <typename TypeName>
    struct remove_const_suffix<TypeName const> {
        using type = TypeName;
    };

    /**
     * Remove const prefix from a type.
     * @tparam TypeName The type to remove const prefix from.
     */
    template <typename TypeName>
    using remove_const_t = typename remove_const<TypeName>::type;

    /**
     * Remove cvref from a type.
     * @tparam FullName The type to remove cvref from.
     */
    template <typename FullName>
    using remove_cvref_t = std::remove_const_t<std::remove_reference_t<FullName>>;

    /**
     * Check if a method has const suffix.
     * @tparam FullName The full name of the method.
     */
    template <typename FullName>
    struct method_has_const_suffix;

    template <typename ReturnType, typename ClassType, typename... ArgTypes>
    struct method_has_const_suffix<ReturnType(ClassType::*)(ArgTypes...) const> {
        static constexpr bool value = true;
    };

    template <typename ReturnType, typename ClassType, typename... ArgTypes>
    struct method_has_const_suffix<ReturnType(ClassType::*)(ArgTypes...)> {
        static constexpr bool value = false;
    };

    /**
     * Extract the return type, argument types, and class type from a method.
     * @tparam FullName The full name of the method.
     */
    template <typename FullName>
    struct extract_method_types;

    template <typename RetType, typename ClassType, typename... ArgTypes>
    struct extract_method_types<RetType(ClassType::*)(ArgTypes...)> {
        using return_type = RetType;
        using arg_types = std::tuple<ArgTypes...>;
        using class_type = ClassType;
    };

    template <typename FullName>
    using extract_method_return_type_t = typename extract_method_types<FullName>::return_type;

    template <typename FullName>
    using extract_method_arg_types_t = typename extract_method_types<FullName>::arg_types;

    template <typename FullName>
    using extract_method_class_type_t = typename extract_method_types<FullName>::class_type;

    /**
     * Check if a std::any instance can be cast into a specific type.
     * @tparam T The type to check.
     */
    template <typename T>
    bool can_cast_to(const std::any& a) {
        return std::any_cast<T>(std::addressof(a)) != nullptr;
    }

    class method_not_found_exception final : public std::exception {
        std::string method_name;

    public:
        explicit method_not_found_exception(std::string method_name) : method_name(std::move(method_name)) {
        }

        [[nodiscard]] const char* what() const noexcept override {
            return ("Method \"" + method_name + "\" not found, or the signature mismatched.").c_str();
        }
    };

    /**
     * A struct to represent a member of a class.
     * @tparam MemberType The type of the member.
     */
    template <typename MemberType>
    struct Member {
        using type = MemberType;
        size_t offset = 0;
        bool is_const = false;

        explicit Member(const size_t offset) : offset(offset) {
        }

        Member(const size_t offset, const bool is_const) : offset(offset), is_const(is_const) {
        }
    };

    /**
     * A struct to represent a method of a class.
     */
    struct CallableWrapper {
        std::any method;
        bool is_const = false;

        explicit CallableWrapper(std::any method) : method(std::move(method)) {
        }

        explicit CallableWrapper(std::any method, const bool is_const) : method(std::move(method)), is_const(is_const) {
        }
    };

    /**
     * A class for reflection.
     */
    class ReflectionBase {
        std::unordered_map<std::string, std::any> m_offsets = {};
        std::unordered_map<std::string, std::pmr::vector<CallableWrapper>> m_funcs = {};

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method(ReturnType (ClassType::*Method)(ArgTypes...)) {
            std::function<ReturnType(void*, ArgTypes...)> fn = [Method](void* object, ArgTypes... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<ArgTypes>(args)...);
            };
            return std::any(std::move(fn));
        }

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method_const(ReturnType (ClassType::*Method)(ArgTypes...) const) {
            std::function<ReturnType(void*, ArgTypes...)> fn = [Method](void* object, ArgTypes... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<ArgTypes>(args)...);
            };
            return std::any(std::move(fn));
        }

    public:
        ReflectionBase() = default;

        /**
         * Register a member of a class.
         * @tparam MemberPtr The pointer to the member.
         * @param name The name of the member.
         */
        template <auto MemberPtr>
        ReflectionBase& register_member(std::string&& name) {
            using ClassType = extract_member_parent_t<decltype(MemberPtr)>;
            const auto offset = reinterpret_cast<size_t>(
                &(
                    static_cast<ClassType *>(nullptr)
                    ->*MemberPtr
                )
            );
            constexpr bool is_const = std::is_const_v<extract_member_type_t<decltype(MemberPtr)>>;
            m_offsets[name] = Member<std::remove_const_t<extract_member_type_t<decltype(MemberPtr)>>>(offset, is_const);

            return *this;
        }

        template <
            typename ReturnType, typename... ArgTypes, typename CallableType,
            std::enable_if_t<std::is_convertible_v<CallableType, std::function<ReturnType(ArgTypes...)>>, bool>  = false
        >
        ReflectionBase& register_function(std::string&& name, CallableType callable) {
            auto fn = static_cast<std::function<ReturnType(remove_cvref_t<ArgTypes>&&...)>>(std::move(callable));
            m_funcs[name].push_back(CallableWrapper(fn));
            return *this;
        }

        /**
         * Find the desired member of a class.
         * @note Returns @b nullptr if the member is not found or there's a type mismatch.
         * @tparam MemberType The pointer to the method.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the method.
         */
        template <typename MemberType, typename ClassType>
        MemberType* get_member_ref(ClassType& object, std::string&& name) noexcept {
            return get_member_ref<MemberType, ClassType>(&object, std::move(name));
        }

        /**
         * Find the desired const member of a class.
         * @note Returns @b nullptr if the member is not found or there's a type mismatch.
         * @tparam MemberType The type of the member.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        template <typename MemberType, typename ClassType>
        const MemberType* get_const_member_ref(ClassType& object, std::string&& name) noexcept {
            return get_const_member_ref<MemberType, ClassType>(&object, std::move(name));
        }

        /**
         * Check if a member is const.
         * @tparam MemberType The type of the member.
         * @param name The name of the member.
         * @return True if the member is const, false otherwise.
         */
        template <typename MemberType>
        bool is_member_const(std::string&& name) noexcept {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                try {
                    using type = remove_const_t<MemberType>;
                    const Member<type> member = std::any_cast<Member<type>>(find->second);
                    return member.is_const;
                } catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        /**
         * Find the desired member of a class.
         * @note Returns @b nullptr if the member is not found or there's a type mismatch.
         * @tparam MemberType The type of the member.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        template <typename MemberType, typename ClassType>
        MemberType* get_member_ref(ClassType* object, std::string&& name) noexcept {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                try {
                    const Member<MemberType> member = std::any_cast<Member<MemberType>>(find->second);
                    return static_cast<MemberType *>(reinterpret_cast<void *>(object) + member.offset);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        /**
         * Find the desired const member of a class.
         * @note Returns @b nullptr if the member is not found or there's a type mismatch.
         * @tparam MemberType The type of the member.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        template <typename MemberType, typename ClassType>
        const MemberType* get_const_member_ref(ClassType* object, std::string&& name) noexcept {
            using type = typename remove_const<MemberType>::type;
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                try {
                    const Member<type> member = std::any_cast<Member<type>>(find->second);
                    return static_cast<const type *>(reinterpret_cast<void *>(object) + member.offset);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        /**
         * Register a method of a class.
         * @note For those methods which have multiple overloads, use another overload of register_method.
         * @tparam Method The pointer to the method.
         * @param name The name of the method.
         */
        template <auto Method>
        ReflectionBase& register_method(std::string&& name) {
            constexpr bool is_const = method_has_const_suffix<decltype(Method)>::value;
            if (m_funcs.find(name) == m_funcs.end()) {
                m_funcs[name] = std::pmr::vector<CallableWrapper>();
            }

            std::any parsed;
            if constexpr (is_const) {
                parsed = _parse_method_const(Method);
            } else {
                parsed = _parse_method(Method);
            }

            m_funcs[name].emplace_back(parsed, is_const);

            return *this;
        }

        /**
         * Register a method of a class.
         * @note This is the overload for those methods which have multiple overloads.
         * @note You need to designate the types of the method manually.
         * @note So if you don't want to do it manually, while your method has no overloads,
         * @note feel free to use the other overload of register_method.
         * @tparam ClassType The type of the class.
         * @tparam ReturnType The return type of the method.
         * @tparam ArgTypes The argument types of the method.
         * @param name The name of the method.
         */
        template <
            typename ClassType,
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ClassType>, bool>  = false
        >
        ReflectionBase& register_method(std::string&& name, ReturnType (ClassType::*Method)(ArgTypes...)) {
            constexpr bool is_const = method_has_const_suffix<decltype(Method)>::value;
            if (m_funcs.find(name) == m_funcs.end()) {
                m_funcs[name] = std::pmr::vector<CallableWrapper>();
            }

            std::any parsed;
            if constexpr (is_const) {
                parsed = _parse_method_const(Method);
            } else {
                parsed = _parse_method(Method);
            }

            m_funcs[name].emplace_back(parsed, is_const);

            return *this;
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @tparam ClassType The type of the class.
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <
            typename ReturnType,
            typename ClassType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            try {
                return invoke_method<ReturnType, ClassType, ArgTypes...>(&object, std::forward<std::string>(name),
                                                 std::forward<ArgTypes>(args)...);
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        template <typename ReturnType, typename ClassType>
        ReturnType invoke_method(ClassType& object, std::string&& name) {
            try {
                return invoke_method<ReturnType, ClassType>(&object, std::forward<std::string>(name));
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        template <typename ClassType>
        void invoke_method(ClassType& object, std::string&& name) {
            try {
                invoke_method<ClassType>(&object, std::forward<std::string>(name));
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ClassType The type of the class.
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <typename ClassType, typename... ArgTypes>
        void invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            try {
                invoke_method<ClassType, ArgTypes...>(&object, std::forward<std::string>(name), std::forward<ArgTypes>(args)...);
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @tparam ClassType The type of the class.
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <
            typename ReturnType,
            typename ClassType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_method(ClassType* object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(void*, ArgTypes...)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<ReturnType(void*, ArgTypes...)>>(fn.method);
                        return method(object, std::forward<ArgTypes>(args)...);
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        template <typename ReturnType, typename ClassType>
        ReturnType invoke_method(ClassType* object, std::string&& name) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(void*)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<ReturnType(void*)>>(fn.method);
                        return method(object);
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ClassType The type of the class.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        template <typename ClassType>
        void invoke_method(ClassType* object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<void(void*)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<void(void*)>>(fn.method);
                        method(object);
                        return;
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ClassType The type of the class.
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <typename ClassType, typename... ArgTypes>
        void invoke_method(ClassType* object, std::string&& name, ArgTypes&&... args) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<void(void*, ArgTypes...)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<void(void*, ArgTypes...)>>(fn.method);
                        method(object, std::forward<ArgTypes>(args)...);
                        return;
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        /**
         * Check if a method is const.
         * @param name The name of the method.
         * @return True if the method is const, false otherwise.
         */
        bool is_method_const(std::string&& name) noexcept {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                try {
                    const auto overloads = find->second;
                    for (auto& fn: overloads) {
                        if (fn.is_const) {
                            return true;
                        }
                    }
                } catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        /**
         * Invoke a function.
         * @exception method_not_found_exception If the function is not found, or the function signature mismatched.
         * @note When calling, the types must perfectly match the function signature.
         * @note For instance, when the type of the function is `float(double, double)`,
         * @note although double can be converted to float,
         * @note you still can't use `invoke_function("func", 1.0f, 1.0f)` to call the original function.
         * @note However, the `const` `volatile` qualifiers, and reference qualifiers are ignored.
         * @tparam ReturnType The return type of the function.
         * @tparam ArgTypes The argument types of the function.
         * @param name The name of the function.
         * @param args The arguments of the function.
         * @return The return value of the function.
         */
        template <
            typename ReturnType, typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_function(std::string&& name, ArgTypes... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(remove_cvref_t<ArgTypes>&&...)>>(fn.method)) {
                        const auto function = std::any_cast<std::function<ReturnType(remove_cvref_t<ArgTypes>&&...)>>(
                            fn.method);
                        return function(std::forward<ArgTypes>(args)...);
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        template <typename ClassType, std::enable_if_t<std::is_default_constructible_v<ClassType>, bool>  = false>
        [[deprecated]] ClassType invoke_ctor() {
            return ClassType();
        }

        template <typename ClassType, typename... ArgTypes>
        [[deprecated]] ClassType invoke_ctor(ArgTypes&&... args) {
            return ClassType(std::forward<ArgTypes>(args)...);
        }
    };

    inline ReflectionBase make_reflection() {
        return {};
    }
}

#endif //SIMPLE_REFL_H
