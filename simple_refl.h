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
#include <vector>

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
    struct remove_const_suffix {
        using type = TypeName;
    };

    template <typename TypeName>
    struct remove_const_suffix<TypeName const> {
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

    template <typename T>
    bool can_cast_to(const std::any& a) {
        return std::any_cast<T>(std::addressof(a)) != nullptr;
    }

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

    class ReflectionBase {
        std::unordered_map<std::string, std::any> m_offsets = {};
        std::unordered_map<std::string, std::pmr::vector<MethodWrapper>> m_funcs = {};

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method(ReturnType(ClassType::*Method)(ArgTypes...)) {
            std::function<ReturnType(void*, ArgTypes...)> fn = [Method](void* object, ArgTypes... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<ArgTypes>(args)...);
            };
            return std::any(std::move(fn));
        }

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method_const(ReturnType(ClassType::*Method)(ArgTypes...) const) {
            std::function<ReturnType(void*, ArgTypes...)> fn = [Method](void* object, ArgTypes... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<ArgTypes>(args)...);
            };
            return std::any(std::move(fn));
        }


    public:
        ReflectionBase() = default;

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

        template <typename MemberType, typename ClassType>
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

        template <typename MemberType, typename ClassType>
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

        template <auto Method>
        ReflectionBase& register_method(std::string&& name) {
            constexpr bool is_const = method_has_const_suffix<decltype(Method)>::value;
            if (m_funcs.find(name) == m_funcs.end()) {
                m_funcs[name] = std::pmr::vector<MethodWrapper>();
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

        template <
            typename ClassType,
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ClassType>, bool> = false
        >
        ReflectionBase& register_method(std::string&& name, ReturnType(ClassType::*Method)(ArgTypes...)) {
            constexpr bool is_const = method_has_const_suffix<decltype(Method)>::value;
            if (m_funcs.find(name) == m_funcs.end()) {
                m_funcs[name] = std::pmr::vector<MethodWrapper>();
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

        template <
            typename ReturnType,
            typename ClassType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool> = false
        >
        ReturnType invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn : fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(void*, ArgTypes...)>>(fn.method)) {
                        auto method = std::any_cast<std::function<ReturnType(void*, ArgTypes...)>>(fn.method);
                        return method(&object, std::forward<ArgTypes>(args)...);
                    }
                }
            }
            throw std::exception();
        }

        template <typename ReturnType, typename ClassType>
        ReturnType invoke_method(ClassType& object, std::string&& name) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn : fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(void*)>>(fn.method)) {
                        auto method = std::any_cast<std::function<ReturnType(void*)>>(fn.method);
                        return method(&object);
                    }
                }
            }
            throw std::exception();
        }

        template <typename ClassType>
        void invoke_method(ClassType& object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn : fn_overloads) {
                    if (can_cast_to<std::function<void(void*)>>(fn.method)) {
                        auto method = std::any_cast<std::function<void(void*)>>(fn.method);
                        method(&object);
                    }
                }
            }
            throw std::exception();
        }

        template <typename ClassType, typename... ArgTypes>
        void invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            if (auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn : fn_overloads) {
                    if (can_cast_to<std::function<void(void*, ArgTypes...)>>(fn.method)) {
                        auto method = std::any_cast<std::function<void(void*, ArgTypes...)>>(fn.method);
                        method(&object, std::forward<ArgTypes>(args)...);
                        return;
                    }
                }
            }
            throw std::exception();
        }

        template <typename... ArgTypes>
        void invoke_method(void* object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn : fn_overloads) {
                    if (auto wrapper = std::any_cast<MethodWrapper>(fn); can_cast_to<void(void*, ArgTypes...)>(wrapper.method)) {
                        auto method = std::any_cast<void(void*, ArgTypes...)>(wrapper.method);
                        return (method)(object, std::forward<ArgTypes>(args)...);
                    }
                }
            }
        }

        bool is_method_const(std::string&& name) noexcept {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                try {
                    const auto overloads = find->second;
                    for (auto& fn : overloads) {
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

        template <typename ClassType, std::enable_if_t<std::is_default_constructible_v<ClassType>, bool>  = false>
        ClassType invoke_ctor() {
            return ClassType();
        }

        template <typename ClassType, typename... ArgTypes>
        ClassType invoke_ctor(ArgTypes&&... args) {
            return ClassType(std::forward<ArgTypes>(args)...);
        }
    };

    inline ReflectionBase make_reflection() {
        return {};
    }
}

#endif //SIMPLE_REFL_H
