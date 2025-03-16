// Simple Reflection Library.
// Created on 2025/3/13.
//
// This library is a simple reflection library designed for C++.
// With it, you can reflect the members and methods of a class, during runtime.

#ifndef SIMPLE_REFL_H
#define SIMPLE_REFL_H

#include <algorithm>
#include <string>
#include <unordered_map>
#include <any>
#include <tuple>
#include <exception>
#include <utility>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <typeindex>
#include <variant>
#include <cstring>

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

    template <typename RetType, typename ClassType, typename... ArgTypes>
    struct extract_method_types<RetType(ClassType::*)(ArgTypes...) const> {
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
            std::stringstream ss;
            ss << "Method \"" << method_name << "\" not found, or the signature mismatched.";
            std::cerr << ss.str() << std::endl;
            return ss.str().c_str();
        }
    };

    struct RawObjectWrapper {
        void* object;
        std::type_index type_index;

        RawObjectWrapper(void* object, std::type_index type_index) : object(object), type_index(type_index) {
        }

        template <typename T>
        explicit RawObjectWrapper(T* object) : object(object), type_index(typeid(T)) {
        }

        [[nodiscard]] bool is_none_type() const {
            return type_index == typeid(void);
        }

        template <typename T>
        [[nodiscard]] bool is_type() const {
            return type_index == typeid(T);
        }

        template <typename T>
        [[nodiscard]] T* into() const {
            if (type_index == typeid(T)) {
                return static_cast<T *>(object);
            }
            return nullptr;
        }

        template <typename T>
        [[nodiscard]] T deref_into() const {
            if (type_index == typeid(T)) {
                return *static_cast<T *>(object);
            }
            throw std::runtime_error("Type mismatch");
        }

        static RawObjectWrapper none() {
            return {nullptr, typeid(void)};
        }
    };

    inline RawObjectWrapper make_raw_object_wrapper(void* object, std::type_index type_index) {
        return {object, type_index};
    }

    template <typename T>
    RawObjectWrapper make_raw_object_wrapper(T* object) {
        return {object, typeid(T)};
    }

    using RawArg = void *;
    using RawArgList = RawArg *;

    class ReturnValueProxy;
    using CommonCallable = std::function<ReturnValueProxy (void*, void** args)>;

    using RawObjectWrapperVec = std::pmr::vector<RawObjectWrapper>;

    struct ArgList {
        RawArgList args;
        std::pmr::vector<std::type_index> type_indices;
        size_t size;

        ArgList(RawArgList args, std::pmr::vector<std::type_index> type_indices, size_t size) {
            this->args = args;
            this->type_indices = std::move(type_indices);
            this->size = size;
        }

        explicit ArgList(const RawObjectWrapperVec& args) {
            this->args = new RawArg[args.size()];
            for (size_t i = 0; i < args.size(); i++) {
                this->args[i] = args[i].object;
                this->type_indices.emplace_back(args[i].type_index);
            }
            this->size = args.size();
        }

        ArgList(const ArgList& other) {
            size = other.size;
            args = static_cast<void **>(std::malloc(sizeof(RawArg) * size));
            std::memcpy(args, other.args, sizeof(RawArg) * other.size);
            type_indices = other.type_indices;
        }

        ~ArgList() {
            delete[] args;
        }

        [[nodiscard]] RawArgList get() const & {
            return args;
        }

        /**
         * This is to prevent calling the get() method on rvalue reference.
         * If not doing so, under certain circumstances, the RawArgList can be invalidated but used afterward.
         * To be specific, the destructor can be, though not always, called right after the get() method,
         * if we don't prevent it.
         */
        [[nodiscard]] RawArgList get() const && = delete;

        [[nodiscard]] std::pmr::vector<RawObjectWrapper> to_object_wrappers() const {
            std::pmr::vector<RawObjectWrapper> wrappers;
            for (size_t i = 0; i < size; i++) {
                wrappers.emplace_back(args[i], type_indices[i]);
            }
            return wrappers;
        }
    };

    inline ArgList refl_arg_list(const RawObjectWrapperVec& args) {
        return ArgList(args);
    }

    template <size_t I = 0, typename... Args>
    constexpr void extract_type_indices(const std::tuple<Args...>&, std::pmr::vector<std::type_index>& indices) {
        if constexpr (I < sizeof...(Args)) {
            indices.emplace_back(typeid(std::tuple_element_t<I, std::tuple<Args...>>));
            extract_type_indices<I + 1>(std::tuple<Args...>{}, indices);
        }
    }

    template <size_t I = 0, typename... Args>
    constexpr void assign_raw_args(RawArgList indices, std::tuple<Args...> args) {
        if constexpr (I < sizeof...(Args)) {
            indices[I] = std::get<I>(args);
            assign_raw_args<I + 1>(indices, args);
        }
    }

    template <typename... ArgTypes>
    ArgList refl_args(ArgTypes&&... args) {
        auto arg_tuple = std::make_tuple(std::addressof(args)...);
        auto arg_list = new RawArg[sizeof...(ArgTypes)];

        assign_raw_args(arg_list, arg_tuple);

        std::pmr::vector<std::type_index> type_indices;
        extract_type_indices(std::tuple<remove_cvref_t<ArgTypes>...>(), type_indices);

        return {arg_list, type_indices, sizeof...(ArgTypes)};
    }

    /**
     * A struct to represent a member of a class.
     * @tparam MemberType The type of the member.
     */
    struct Member {
        size_t offset = 0;
        bool is_const = false;
        const std::type_info& type_info;

        explicit Member(const size_t offset, const std::type_info& type_info) : offset(offset), type_info(type_info) {
        }

        Member(const size_t offset, const bool is_const, const std::type_info& type_info) : offset(offset),
            is_const(is_const), type_info(type_info) {
        }
    };

    /**
     * A struct to represent a method of a class.
     */
    struct CallableWrapper {
        // compatibility for API of older versions.
        std::any method;
        bool is_const = false;

        CommonCallable callable;
        std::type_index return_type;
        std::pmr::vector<std::type_index> arg_types;
        std::variant<std::type_index, std::monostate> parent_type;

        CallableWrapper(
            std::any method,
            CommonCallable callable,
            std::type_index return_type,
            std::pmr::vector<std::type_index>&& arg_types,
            std::variant<std::type_index, std::monostate> parent_type,
            bool is_const = false
        ) : method(std::move(method)),
            callable(std::move(callable)),
            return_type(return_type),
            arg_types(std::move(arg_types)),
            parent_type(parent_type),
            is_const(is_const) {
        }
    };

    class ReturnValueProxy {
        std::shared_ptr<void> ptr;
        size_t size;
        std::type_index type_index = typeid(void);

    public:
        template <typename ValueType>
        explicit ReturnValueProxy(ValueType&& ptr): type_index(typeid(ValueType)) {
            this->ptr = std::make_shared<ValueType>(std::forward<ValueType>(ptr));
            this->size = sizeof(ValueType);
        }

        template <typename ValueType>
        ValueType get() {
            return *static_cast<ValueType *>(this->ptr.get());
        }

        [[nodiscard]] void* get_raw() const {
            return this->ptr.get();
        }

        [[nodiscard]] size_t get_size() const {
            return this->size;
        }

        [[nodiscard]] std::type_index get_type_index() const {
            return this->type_index;
        }

        [[nodiscard]] RawObjectWrapper to_object_wrapper() const {
            return {this->get_raw(), this->get_type_index()};
        }
    };

    // wtf is this!?
    template <typename ReturnType, typename ClassType, typename... ArgTypes, size_t... Indices>
    auto wrap_method_impl(ReturnType (ClassType::*method)(ArgTypes...), std::index_sequence<Indices...>) {
        return std::function<ReturnValueProxy (void*, RawArgList args)>(
            // here we use a lambda function to wrap the method,
            // the lambda takes a void* object and a void** args,
            // the void* is the pointer to the object, whose method is about to be invoked,
            // while the void** args is the array of arguments.
            [method](void* object, RawArgList args) -> ReturnValueProxy {
                // cast the void* object to the correct type.
                auto cls = static_cast<ClassType *>(object);
                // invoke the method, and return the result.
                // return the result in the form of a ReturnValueProxy, which is a wrapper for the return value.
                // type-erased, and can manage life cycle of the return value.
                if constexpr (std::is_void_v<ReturnType>) {
                    (cls->*method)(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(*(args + Indices)))...);
                    // if the return type is void, return a zero value, which is a nullptr.
                    return ReturnValueProxy(0);
                } else {
                    auto ret = (cls->*method)(
                        // use std::forward to forward the arguments to the method.
                        std::forward<remove_cvref_t<ArgTypes>>(
                            // cast the void* to the correct type,
                            // and then dereference it to get the value.
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(
                                // as mentioned above, the void** args is an array of pointers,
                                // so here we use an offset to get the pointer to a specific argument.
                                // and then dereference it to get the value.
                                *(args + Indices)
                            )
                        )... // argument pack.
                    );
                    return ReturnValueProxy(std::move(ret));
                }
            });
    }

    template <typename ReturnType, typename ClassType, typename... ArgTypes>
    auto wrap_method(ReturnType (ClassType::*method)(ArgTypes...)) {
        return wrap_method_impl(method, std::make_index_sequence<sizeof...(ArgTypes)>{});
    }

    template <typename ReturnType, typename ClassType, typename... ArgTypes, size_t... Indices>
    auto wrap_method_const_impl(ReturnType (ClassType::*method)(ArgTypes...) const, std::index_sequence<Indices...>) {
        return std::function<ReturnValueProxy (void*, RawArgList args)>(
            [method](void* object, RawArgList args) -> ReturnValueProxy {
                const auto cls = static_cast<ClassType *>(object);
                if constexpr (std::is_void_v<ReturnType>) {
                    (cls->*method)(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(*(args + Indices)))...);
                    return ReturnValueProxy(0);
                } else {
                    auto ret = (cls->*method)(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(
                                *(args + Indices)))...);
                    return ReturnValueProxy(std::move(ret));
                }
            });
    }

    template <typename ReturnType, typename ClassType, typename... ArgTypes>
    auto wrap_method_const(ReturnType (ClassType::*method)(ArgTypes...) const) {
        return wrap_method_const_impl(method, std::make_index_sequence<sizeof...(ArgTypes)>{});
    }

    /**
     * Wrap a function into a std::function object.
     * @note Note that the wrapped function takes an additional void pointer as the 1st argument.
     * @note This is to make the behavior consistent with the wrapped method.
     * @tparam ReturnType The return type of the function.
     * @tparam ArgTypes The argument types of the function.
     * @tparam Indices
     * @param function The function to be wrapped.
     * @return A std::function object that wraps the function.
     */
    template <typename ReturnType, typename... ArgTypes, size_t... Indices>
    auto wrap_function_impl(std::function<ReturnType (ArgTypes...)> function, std::index_sequence<Indices...>) {
        return std::function<ReturnValueProxy (void*, RawArgList args)>(
            [function](void* placeholder, RawArgList args) -> ReturnValueProxy {
                if constexpr (std::is_void_v<ReturnType>) {
                    function(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(*(args + Indices)))...);
                    return ReturnValueProxy(0);
                } else {
                    auto ret = function(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(
                                *(args + Indices)))...);
                    return ReturnValueProxy(std::move(ret));
                }
            });
    }

    template <typename ReturnType, typename... ArgTypes>
    auto wrap_function(std::function<ReturnType (ArgTypes...)> function) {
        return wrap_function_impl(function, std::make_index_sequence<sizeof...(ArgTypes)>{});
    }

    /**
     * A class for reflection.
     */
    class ReflectionBase {
        std::unordered_map<std::string, Member> m_offsets = {};
        std::unordered_map<std::string, std::pmr::vector<CallableWrapper>> m_funcs = {};

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method(ReturnType (ClassType::*Method)(ArgTypes...)) {
            std::function<ReturnType(void*, remove_cvref_t<ArgTypes>&&...)> fn =
                    [Method](void* object, remove_cvref_t<ArgTypes>&&... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<remove_cvref_t<ArgTypes>>(args)...);
            };
            return std::any(std::move(fn));
        }

        template <typename ClassType, typename ReturnType, typename... ArgTypes>
        std::any _parse_method_const(ReturnType (ClassType::*Method)(ArgTypes...) const) {
            std::function<ReturnType(void*, remove_cvref_t<ArgTypes>&&...)> fn =
                    [Method](void* object, remove_cvref_t<ArgTypes>&&... args) {
                return (static_cast<ClassType *>(object)->*Method)(std::forward<remove_cvref_t<ArgTypes>>(args)...);
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
            using MemberType = extract_member_type_t<decltype(MemberPtr)>;
            const auto offset = reinterpret_cast<size_t>(
                &(
                    static_cast<ClassType *>(nullptr)
                    ->*MemberPtr
                )
            );
            constexpr bool is_const = std::is_const_v<extract_member_type_t<decltype(MemberPtr)>>;
            m_offsets.emplace(std::move(name), Member(offset, is_const, typeid(MemberType)));

            return *this;
        }

        template <
            typename ReturnType, typename... ArgTypes, typename CallableType,
            std::enable_if_t<std::is_convertible_v<CallableType, std::function<ReturnType(ArgTypes...)>>, bool>  = false
        >
        ReflectionBase& register_function(std::string&& name, CallableType callable) {
            auto fn = static_cast<std::function<ReturnType(remove_cvref_t<ArgTypes>&&...)>>(std::move(callable));
            auto wrapped_fn = wrap_function(fn);
            m_funcs[name].emplace_back(
                std::move(fn),
                wrapped_fn,
                typeid(ReturnType),
                std::pmr::vector<std::type_index>{typeid(ArgTypes)...},
                std::monostate(),
                false);
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
                const Member& member = find->second;
                if (member.type_info != typeid(MemberType)) {
                    return false;
                }
                return member.is_const;
            }
            return false;
        }

        bool is_member_const(std::string&& name) noexcept {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const Member& member = find->second;
                return member.is_const;
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
                const auto& member = find->second;
                if (member.type_info != typeid(MemberType)) {
                    return nullptr;
                }
                return static_cast<MemberType *>(reinterpret_cast<void *>(object) + member.offset);
            }
            return nullptr;
        }

        /**
         * Provides direct access to the pointer of the desired member, with no type safety guarantees.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        void* get_member_ref(void* object, std::string&& name) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                return object + member.offset;
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
                const auto& member = find->second;
                if (member.type_info != typeid(type)) {
                    return nullptr;
                }
                return static_cast<const type *>(reinterpret_cast<void *>(object) + member.offset);
            }
            return nullptr;
        }

        /**
         * Provides direct access to the desired const member of a class, with no type safety guarantees.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        const void* get_const_member_ref(const void* object, std::string&& name) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                return object + member.offset;
            }
            return nullptr;
        }

        RawObjectWrapper get_member_as_wrapped_object(void* object, std::string&& name) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                return RawObjectWrapper(object + member.offset, member.type_info);
            }
            return RawObjectWrapper::none();
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
            using ReturnType = typename extract_method_types<decltype(Method)>::return_type;
            using ArgTypes = typename extract_method_types<decltype(Method)>::arg_types;
            using ClassType = typename extract_method_types<decltype(Method)>::class_type;

            std::pmr::vector<std::type_index> arg_types;
            ArgTypes arg_types_tuple;
            extract_type_indices(arg_types_tuple, arg_types);

            if (m_funcs.find(name) == m_funcs.end()) {
                m_funcs[name] = std::pmr::vector<CallableWrapper>();
            }

            CommonCallable parsed;
            std::any old_parsed;

            if constexpr (is_const) {
                parsed = wrap_method_const(Method);
                old_parsed = _parse_method_const(Method);
            } else {
                parsed = wrap_method(Method);
                old_parsed = _parse_method(Method);
            }

            m_funcs[name].emplace_back(
                old_parsed,
                parsed,
                std::type_index(typeid(ReturnType)),
                std::move(arg_types),
                std::type_index(typeid(ClassType)),
                is_const
            );

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

            CommonCallable parsed;
            std::any old_parsed;

            if constexpr (is_const) {
                parsed = wrap_method_const(Method);
                old_parsed = _parse_method_const(Method);
            } else {
                parsed = wrap_method(Method);
                old_parsed = _parse_method(Method);
            }

            std::pmr::vector<std::type_index> arg_types;
            std::tuple<remove_cvref_t<ArgTypes>...> arg_types_tuple;
            extract_type_indices(arg_types_tuple, arg_types);

            m_funcs[name].emplace_back(
                old_parsed,
                parsed,
                std::type_index(typeid(ReturnType)),
                std::move(arg_types),
                std::type_index(typeid(ClassType)),
                is_const
            );
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
                return invoke_method<ReturnType, remove_cvref_t<ArgTypes>...>(
                    &object, std::forward<std::string>(name),
                    std::forward<remove_cvref_t<ArgTypes>>(args)...);
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
        template <typename ReturnType, typename ClassType, std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false>
        ReturnType invoke_method(ClassType& object, std::string&& name) {
            try {
                return invoke_method<ReturnType>(&object, std::forward<std::string>(name));
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
                invoke_method(&object, std::forward<std::string>(name));
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
        template <
            typename ReturnType, typename ClassType, typename... ArgTypes,
            std::enable_if_t<std::is_void_v<ReturnType>, bool>  = false
        >
        void invoke_method(ClassType& object, std::string&& name, ArgTypes&&... args) {
            try {
                invoke_method<void, ArgTypes...>(&object, std::forward<std::string>(name),
                                                 std::forward<remove_cvref_t<ArgTypes>>(args)...);
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <
            typename ReturnType,
            typename... ArgTypes,
            std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false
        >
        ReturnType invoke_method(void* object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<ReturnType(void*, remove_cvref_t<ArgTypes>&&...)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<ReturnType
                            (void*, remove_cvref_t<ArgTypes>&&...)>>(fn.method);
                        return method(object, std::forward<remove_cvref_t<ArgTypes>>(args)...);
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        /**
         * Invoke a method of a class.
         * @exception method_not_found_exception If the method is not found.
         * @tparam ReturnType The return type of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        template <typename ReturnType>
        ReturnType invoke_method(void* object, std::string&& name) {
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
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @return The return value of the method.
         */
        void invoke_method(void* object, std::string&& name) {
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
         * @tparam ArgTypes The argument types of the method.
         * @param object The pointer to the object.
         * @param name The name of the method.
         * @param args The arguments of the method.
         * @return The return value of the method.
         */
        template <typename ReturnType, typename... ArgTypes, std::enable_if_t<std::is_void_v<ReturnType>, bool>  =
                false>
        void invoke_method(void* object, std::string&& name, ArgTypes&&... args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<void(void*, remove_cvref_t<ArgTypes>&&...)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<void(void*, remove_cvref_t<ArgTypes>&&...)>>(
                            fn.method);
                        method(object, std::forward<remove_cvref_t<ArgTypes>>(args)...);
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

        ReturnValueProxy invoke_function(std::string&& name, const ArgList& args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (is_parameter_match(fn.arg_types, args.type_indices)) {
                        // the nullptr here serves as a placeholder, since we don't need to pass the object to the function.
                        ReturnValueProxy proxy = fn.callable(nullptr, args.get());
                        return proxy;
                    }
                }
            }
            throw method_not_found_exception(name);
        }

        static bool is_parameter_match(
            const std::pmr::vector<std::type_index>& parameters,
            const std::pmr::vector<std::type_index>& actual_args) {
            if (parameters.size() != actual_args.size()) {
                return false;
            }

            for (size_t i = 0; i < parameters.size(); ++i) {
                if (parameters[i] != actual_args[i]) {
                    return false;
                }
            }
            return true;
        }

        template <typename ClassType, std::enable_if_t<!std::is_void_v<ClassType>, bool>  = false>
        ReturnValueProxy invoke_method(ClassType& object, std::string&& name, ArgList args) {
            try {
                return invoke_method(&object, std::move(name), std::move(args));
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        template <typename ClassType, std::enable_if_t<!std::is_void_v<ClassType>, bool>  = false>
        ReturnValueProxy invoke_method(ClassType* object, std::string&& name, ArgList args) {
            try {
                return invoke_method(static_cast<void *>(object), std::move(name), std::move(args));
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        template <typename ClassType, std::enable_if_t<std::is_void_v<ClassType>, bool>  = false>
        ReturnValueProxy invoke_method(ClassType* object, std::string&& name, const ArgList& args) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    const std::pmr::vector<std::type_index>& arg_types = args.type_indices;
                    if (is_parameter_match(fn.arg_types, arg_types)) {
                        ReturnValueProxy proxy = fn.callable(object, args.get());
                        return proxy;
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
