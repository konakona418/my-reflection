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
    };

    template <typename FullName>
    using extract_member_type_t = typename extract_member_type<FullName>::type;

    template <typename TypeName>
    struct remove_const {
        using type = TypeName;
    };

    template <typename TypeName>
    struct remove_const<const TypeName> {
        using type = TypeName;
    };

    template <typename FullName>
    struct extract_method_types;

    template <typename RetType, typename ClassType, typename... ArgTypes>
    struct extract_method_types<RetType(ClassType::*)(ArgTypes...)> {
        using return_type = RetType;
        using arg_types = std::tuple<ArgTypes...>;
        using class_type = ClassType;
    };

    template <typename MemberType>
    struct Member {
        using type = MemberType;
        size_t offset = 0;

        explicit Member(const size_t offset) : offset(offset) {
        }
    };

    template <typename ClassType>
    class ReflectionBase {
        std::unordered_map<std::string, std::any> m_offsets = {};
        std::unordered_map<std::string, std::any> m_funcs = {};

    public:
        ReflectionBase() = default;

        template <auto MemberPtr>
        void register_member(std::string&& name) {
            const auto offset = reinterpret_cast<size_t>(
                &(
                    static_cast<ClassType *>(nullptr)
                    ->*MemberPtr
                )
            );
            m_offsets[name] = Member<extract_member_type_t<decltype(MemberPtr)>>(offset);
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
                    const Member<const type> member = std::any_cast<Member<const type>>(find->second);
                    return static_cast<const type *>(reinterpret_cast<void *>(&object) + member.offset);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }

        template <auto Method>
        void register_method(std::string&& name) {
            m_funcs[name] = std::any(Method);
        }

        template <
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool> = false
        >
        ReturnType invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)(ArgTypes...)>(find->second);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        template <typename ReturnType>
        ReturnType invoke_method(ClassType& object, std::string&& name) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<ReturnType(ClassType::*)(void)>(find->second);
                return (object.*fn)();
            }
            throw std::exception();
        }

        void invoke_method(ClassType& object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)(void)>(find->second);
                return (object.*fn)();
            }
            throw std::exception();
        }

        template <typename... ArgTypes>
        void invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn = std::any_cast<void(ClassType::*)(ArgTypes...)>(find->second);
                return (object.*fn)(std::forward<ArgTypes>(args)...);
            }
            throw std::exception();
        }

        template <std::enable_if_t<std::is_default_constructible_v<ClassType>, bool> = false>
        ClassType invoke_ctor() {
            return ClassType();
        }

        template <typename... ArgTypes>
        ClassType invoke_ctor(ArgTypes&&... args) {
            return ClassType(std::forward<ArgTypes>(args)...);
        }
    };
}

#endif //SIMPLE_REFL_H
