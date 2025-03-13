//
// Created on 2025/3/13.
//

#ifndef SIMPLE_REFL_H
#define SIMPLE_REFL_H

#include <string>
#include <unordered_map>
#include <any>
#include <tuple>
#include <exception>
#include <utility>

namespace simple_reflection {
    template <typename FullName>
    struct extract_member_type;

    template <typename ClassType, typename MemberType>
    struct extract_member_type<MemberType ClassType::*> {
        using type = MemberType;
        using parent_type = ClassType;
    };

    template <typename FullName>
    using extract_member_type_t = typename extract_member_type<FullName>::type;

    template <typename FullName>
    using extract_member_parent_t = typename extract_member_type<FullName>::parent_type;

    template <typename TypeName>
    struct remove_const {
        using type = TypeName;
    };

    template <typename TypeName>
    struct remove_const<const TypeName> {
        using type = TypeName;
    };

    template <typename TypeName>
    using remove_const_t = typename remove_const<TypeName>::type;

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

    struct MethodWrapper {
        std::any method;
        bool is_const = false;

        explicit MethodWrapper(std::any method) : method(std::move(method)) {
        }

        explicit MethodWrapper(std::any method, const bool is_const) : method(std::move(method)), is_const(is_const) {
        }
    };

    template <typename ClassType>
    class ReflectionBase {
        std::unordered_map<std::string, std::any> m_offsets = {};
        std::unordered_map<std::string, std::any> m_funcs = {};

    public:
        ReflectionBase() = default;

        template <auto MemberPtr>
        ReflectionBase& register_member(std::string&& name) {
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

        template <typename MemberType>
        MemberType* get_member_ref(ClassType& object, std::string&& name) noexcept {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                try {
                    const Member<MemberType> member = std::any_cast<Member<MemberType>>(find->second);
                    return static_cast<MemberType *>(reinterpret_cast<void *>(&object) + member.offset);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        template <typename MemberType>
        const MemberType* get_const_member_ref(ClassType& object, std::string&& name) noexcept {
            using type = typename remove_const<MemberType>::type;
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                try {
                    const Member<type> member = std::any_cast<Member<type>>(find->second);
                    return static_cast<const type *>(reinterpret_cast<void *>(&object) + member.offset);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        template <typename MemberType>
        bool is_member_const(ClassType& object, std::string&& name) noexcept {
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

        template <auto Method>
        ReflectionBase& register_method(std::string&& name) {
            constexpr bool is_const = method_has_const_suffix<decltype(Method)>::value;
            m_funcs[name] = std::any(MethodWrapper(Method, is_const));

            return *this;
        }

        template <
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)(ArgTypes...)>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        template <typename ReturnType>
        ReturnType invoke_method(ClassType& object, std::string&& name) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)()>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)();
            }
            throw std::exception();
        }

        void invoke_method(ClassType& object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)()>(std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)();
            }
            throw std::exception();
        }

        template <typename... ArgTypes>
        void invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)(ArgTypes...)>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        template <
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_const_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)(ArgTypes...) const>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        bool is_method_const(ClassType& object, std::string&& name) noexcept {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                try {
                    const auto method = std::any_cast<MethodWrapper>(find->second);
                    return std::any_cast<bool>(method.is_const);
                } catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        template <typename ReturnType>
        ReturnType invoke_const_method(ClassType& object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)(void) const>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)();
            }
            throw std::exception();
        }

        void invoke_const_method(ClassType& object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)(void) const>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)();
            }
            throw std::exception();
        }

        template <typename... ArgTypes>
        void invoke_const_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)(ArgTypes...) const>(
                    std::any_cast<MethodWrapper>(find->second).method);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        template <std::enable_if_t<std::is_default_constructible_v<ClassType>, bool>  = false>
        ClassType invoke_ctor() {
            return ClassType();
        }

        template <typename... ArgTypes>
        ClassType invoke_ctor(ArgTypes&&... args) {
            return ClassType(std::forward<ArgTypes>(args)...);
        }
    };

    template <typename ClassType>
    ReflectionBase<ClassType> make_reflection() {
        return ReflectionBase<ClassType>();
    }
}

#endif //SIMPLE_REFL_H
