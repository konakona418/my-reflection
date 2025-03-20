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
#include <numeric>
#include <cstring>
#include <map>
#include <stack>

#define make_args(...) simple_reflection::refl_args(__VA_ARGS__)

/** Simple Reflection Library. */
namespace simple_reflection {
    class ReflectionBase;
    class ReflectionRegistryBase;

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

    inline bool string_contains(const std::string& full_string, const std::string& sub_string) {
        return full_string.find(sub_string) != std::string::npos;
    }

    inline std::string _remove_redundant_space(const std::string& full_string) {
        std::string result = full_string;
        while (result.find(' ') != std::string::npos) {
            result.erase(result.find(' '), 1);
        }
        return result;
    }

    inline std::string _string_replace(std::string& full_string, const std::string& sub_string,
                                       const std::string& replacement) {
        return full_string.replace(full_string.find(sub_string), sub_string.length(), replacement);
    }

    inline std::string _extract_type(std::string& full_string) {
        size_t begin = full_string.find("[with T = ") + 9;
        size_t end = full_string.find(']');

        std::vector<size_t> semicolons;
        for (size_t i = begin; i < end; ++i) {
            if (full_string[i] == ';') {
                semicolons.push_back(i);
            }
        }

        std::vector<std::string> segments;
        for (size_t i = 0; i < semicolons.size(); ++i) {
            size_t start = i == 0 ? begin : semicolons[i - 1] + 1;
            size_t stop = semicolons[i];
            segments.push_back(_remove_redundant_space(full_string.substr(start, stop - start)));
        }
        segments.push_back(
            _remove_redundant_space(full_string.substr(semicolons.back() + 1, end - semicolons.back() - 1)));

        std::string base_type = _remove_redundant_space(segments[0]);
        std::map<std::string, std::string> aliases;
        aliases["T"] = base_type;
        for (size_t i = 1; i < segments.size(); ++i) {
            size_t equal_sign = segments[i].find('=');
            std::string alias = _remove_redundant_space(segments[i].substr(0, equal_sign));
            std::string type = _remove_redundant_space(segments[i].substr(equal_sign + 1));
            aliases[alias] = type;
        }


        for (auto it = aliases.rbegin(); it != aliases.rend(); ++it) {
            for (auto it2 = aliases.rbegin(); it2 != aliases.rend(); ++it2) {
                if (it == it2) continue;
                if (string_contains(it2->second, it->second)) {
                    _string_replace(it2->second, it->second, it->first);
                }
            }
        }
        return aliases["T"];
    }

    template <typename T>
    std::string extract_type_name() {
        std::string fn_sig;
        std::string type_name;
#if defined (__GNUC__) || defined (__clang__)
        fn_sig = __PRETTY_FUNCTION__;
        type_name = _extract_type(fn_sig);
#elif defined (_MSC_VER)
        fn_sig = __FUNCSIG__;
        size_t start = fn_sig.find("extract_type_name<") + 13;
        size_t end = fn_sig.find(">(void)");
        type_name = fn_sig.substr(start, end - start);
#else
        return std::string(typeid(T).name());
#endif
        return type_name;
    }

    struct ParsedTypeString {
        std::string type_name;
        std::vector<std::string> namespaces;

        std::vector<ParsedTypeString> templates;

        friend std::ostream& operator<<(std::ostream& os, const ParsedTypeString& pts) {
            for (auto& ns: pts.namespaces) {
                os << ns << "::";
            }
            os << pts.type_name;
            for (auto& t: pts.templates) {
                os << "<" << t << ">";
            }
            return os;
        }

        [[nodiscard]] std::string as_readable_format() const {
            std::stringstream ss;
            ss << this->type_name;
            if (!this->templates.empty()) {
                ss << "<";
                for (size_t i = 0; i < this->templates.size(); ++i) {
                    ss << 'T' << i;
                    if (i != this->templates.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ">";
            }
            ss << " [of namespace ";

            for (size_t i = 0; i < this->namespaces.size(); ++i) {
                ss << this->namespaces[i];
                if (i != this->namespaces.size() - 1) {
                    ss << "::";
                }
            }

            if (!this->templates.empty()) {
                ss << "; with template args ";
                for (size_t i = 0; i < this->templates.size(); ++i) {
                    ss << "T" << i << " = ";
                    ss << this->templates[i];
                    if (i != this->templates.size() - 1) {
                        ss << ", ";
                    }
                }
            }
            ss << "]";
            return ss.str();
        }
    };

    inline ParsedTypeString parse_type_string(const std::string& type) {
        ParsedTypeString result;

        auto type_string = type;

        if (type_string.find('<') != std::string::npos) {
            const size_t begin = type_string.find('<') + 1;
            const size_t end = type_string.find('>');

            const std::string type_args = type_string.substr(begin, end - begin);
            type_string = type_string.substr(0, begin - 1);

            std::vector<size_t> commas;
            for (size_t i = 0; i < type_args.size(); ++i) {
                if (type_args[i] == ',') {
                    commas.push_back(i);
                }
            }

            if (!commas.empty()) {
                for (size_t i = 0; i < commas.size(); ++i) {
                    const size_t start = i == 0 ? begin : commas[i - 1] + 1;
                    const size_t stop = commas[i];
                    result.templates.push_back(parse_type_string(type_args.substr(start, stop - start)));
                }
            } else {
                result.templates.push_back(parse_type_string(type_args));
            }
        }

        if (type_string.find("::") != std::string::npos) {
            size_t first_colon = type_string.find_first_of("::");
            while (first_colon != std::string::npos) {
                result.namespaces.push_back(type_string.substr(0, first_colon));
                type_string = type_string.substr(first_colon + 2);
                first_colon = type_string.find_first_of("::");
            }
        }

        result.type_name = type_string;
        return result;
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

    class metadata_not_found_exception : public std::exception {
        std::string metadata_name;

    public:
        explicit metadata_not_found_exception(std::string metadata_name) : metadata_name(std::move(metadata_name)) {
        }

        [[nodiscard]] const char* what() const noexcept override {
            std::stringstream ss;
            ss << "Metadata \"" << metadata_name << "\" not found.";
            std::cerr << ss.str() << std::endl;
            return ss.str().c_str();
        }
    };

    struct SharedObjectWrapper {
        std::shared_ptr<void> object;
        std::type_index type_index;

        SharedObjectWrapper(std::shared_ptr<void> object, std::type_index type_index)
            : object(std::move(object)), type_index(type_index) {
        }

        template <typename T>
        explicit SharedObjectWrapper(std::shared_ptr<T> object) : object(std::move(object)), type_index(typeid(T)) {
        }

        template <typename T>
        [[nodiscard]] T* into() const {
            if (type_index == typeid(T)) {
                return static_cast<T *>(object.get());
            }
            return nullptr;
        }

        template <typename T>
        [[nodiscard]] T deref_into() const {
            if (type_index == typeid(T)) {
                return *static_cast<T *>(object.get());
            }
            return T();
        }

        template <typename T>
        [[nodiscard]] bool is_type() const {
            return type_index == typeid(T);
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

        template <typename T>
        [[nodiscard]] std::shared_ptr<T> into_shared() {
            if (type_index == typeid(T)) {
                return std::make_shared<T>(*static_cast<T *>(object));
            }
            throw std::runtime_error("Type mismatch");
        }

        template <typename T>
        T set_value(T value) {
            if (type_index == typeid(T)) {
                *static_cast<T *>(object) = value;
                return value;
            }
            throw std::runtime_error("Type mismatch");
        }

        static RawObjectWrapper none() {
            return {nullptr, typeid(void)};
        }
    };

    inline RawObjectWrapper wrap_object(void* object, std::type_index type_index) {
        return {object, type_index};
    }

    template <typename T>
    RawObjectWrapper wrap_object(T* object) {
        return {object, typeid(T)};
    }

    template <typename T>
    RawObjectWrapper wrap_object(T&& object) {
        return {std::addressof(object), typeid(T)};
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

        ArgList(std::initializer_list<RawObjectWrapper> args) {
            this->args = new RawArg[args.size()];
            for (size_t i = 0; i < args.size(); i++) {
                this->args[i] = args.begin()[i].object;
                this->type_indices.emplace_back(args.begin()[i].type_index);
            }
            this->size = args.size();
        }

        ArgList(const ArgList& other) = delete;

        ArgList(ArgList&& other) noexcept {
            args = other.args;
            type_indices = std::move(other.type_indices);
            size = other.size;
            other.args = nullptr;
        }

        ~ArgList() {
            delete[] args;
        }

        friend ArgList operator|(ArgList&& lhs, ArgList&& rhs) {
            auto merged_args = new RawArg[lhs.size + rhs.size];

            for (size_t i = 0; i < lhs.size; i++) {
                merged_args[i] = lhs.args[i];
            }
            for (size_t i = 0; i < rhs.size; i++) {
                merged_args[i + lhs.size] = rhs.args[i];
            }

            auto type_indices = lhs.type_indices;
            type_indices.insert(type_indices.end(), rhs.type_indices.begin(), rhs.type_indices.end());

            delete[] lhs.args;
            delete[] rhs.args;

            lhs.args = nullptr;
            rhs.args = nullptr;

            return {merged_args, type_indices, lhs.size + rhs.size};
        }

        friend ArgList operator,(ArgList&& lhs, ArgList&& rhs) {
            return std::move(lhs) | std::move(rhs);
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

        static ArgList empty() {
            return {nullptr, {}, 0};
        }

        friend ArgList operator|(ArgList&& lhs, RawObjectWrapper rhs) {
            return std::move(lhs) | ArgList(std::initializer_list{rhs});
        }

        friend ArgList operator|(RawObjectWrapper lhs, ArgList&& rhs) {
            return ArgList(std::initializer_list{lhs}) | std::move(rhs);
        }

        friend ArgList operator|(ArgList&& lhs, const RawObjectWrapperVec& rhs) {
            return std::move(lhs) | ArgList(rhs);
        }

        friend ArgList operator|(const RawObjectWrapperVec& lhs, ArgList&& rhs) {
            return ArgList(lhs) | std::move(rhs);
        }

        friend ArgList operator,(ArgList&& lhs, RawObjectWrapper rhs) {
            return std::move(lhs) | ArgList(std::initializer_list{rhs});
        }

        friend ArgList operator,(RawObjectWrapper lhs, ArgList&& rhs) {
            return ArgList(std::initializer_list{lhs}) | std::move(rhs);
        }

        friend ArgList operator,(ArgList&& lhs, const RawObjectWrapperVec& rhs) {
            return std::move(lhs) | ArgList(rhs);
        }

        friend ArgList operator,(const RawObjectWrapperVec& lhs, ArgList&& rhs) {
            return ArgList(lhs) | std::move(rhs);
        }
    };

    inline ArgList refl_arg_list(const RawObjectWrapperVec& args) {
        return ArgList(args);
    }

    inline ArgList empty_arg_list() {
        return ArgList::empty();
    }

    /**
     * Merge two ArgList into one.
     * @note This will invalidate the ArgList passed in.
     * @param lhs The first ArgList.
     * @param rhs The second ArgList.
     * @return The merged ArgList.
     */
    inline ArgList merge_arg_list(ArgList&& lhs, ArgList&& rhs) {
        return std::move(lhs) | std::move(rhs);
    }

    /**
     * Merge multiple ArgList into one.
     * @note This will invalidate the ArgList passed in.
     * @tparam ArgTypes
     * @param args The ArgList to be merged.
     * @return The merged ArgList.
     */
    template <
        typename... ArgTypes,
        std::enable_if_t<(std::is_same_v<remove_cvref_t<ArgTypes>, ArgList> && ...), bool>  = false,
        std::enable_if_t<sizeof...(ArgTypes) >= 2, bool>  = false
    >
    ArgList merge_arg_list(ArgTypes&&... args) {
        std::vector<ArgList> arg_lists;
        (arg_lists.emplace_back(std::forward<ArgTypes>(args)), ...);

        size_t size = std::accumulate(
            arg_lists.begin(), arg_lists.end(), 0,
            [](size_t sum, const ArgList& arg_list) {
                return sum + arg_list.size;
            });

        auto* merged_args = new RawArg[size];
        size_t offset = 0;
        for (auto& arg_list: arg_lists) {
            for (size_t j = 0; j < arg_list.size; j++) {
                merged_args[offset++] = arg_list.args[j];
            }
        }

        for (auto& arg_list: arg_lists) {
            delete[] arg_list.args;
            arg_list.args = nullptr;
        }

        auto type_indices = arg_lists[0].type_indices;
        for (size_t i = 1; i < arg_lists.size(); i++) {
            type_indices.insert(
                type_indices.end(), arg_lists[i].type_indices.begin(),
                arg_lists[i].type_indices.end());
        }

        return {merged_args, type_indices, size};
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
        size_t size = 0;
        bool is_const = false;
        const std::type_info& type_info;

        std::function<void(void*, void*)> setter;

        template <typename MemberType>
        Member init_setter() {
            size_t offset = this->offset;
            size_t size = sizeof(MemberType);
            bool is_const = this->is_const;
            setter = [offset, size, is_const](void* obj, void* arg) {
                if (is_const) {
                    throw std::runtime_error("Cannot assign to const member");
                }
                if constexpr (std::is_copy_assignable_v<MemberType>) {
                    auto* member = const_cast<remove_const_t<MemberType> *>(static_cast<MemberType *>(
                        obj + offset));
                    *member = *static_cast<MemberType *>(arg);
                } else if constexpr (std::is_trivially_copyable_v<MemberType>) {
                    auto* member = obj + offset;
                    std::memcpy(member, arg, size);
                } else {
                    throw std::runtime_error("MemberType is neither copy assignable nor trivially copyable");
                }
            };
            return *this;
        }

        explicit Member(const size_t offset, const size_t size, const std::type_info& type_info)
            : offset(offset), size(size), type_info(type_info) {
        }

        Member(const size_t offset, const size_t size, const bool is_const, const std::type_info& type_info)
            : offset(offset), size(size), is_const(is_const), type_info(type_info) {
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
            is_const(is_const),
            callable(std::move(callable)),
            return_type(return_type),
            arg_types(std::move(arg_types)),
            parent_type(parent_type) {
        }
    };

    struct Metadata {
        std::type_index type_index;
        std::any data;

        Metadata(std::type_index type_index, std::any data) : type_index(type_index), data(std::move(data)) {
        }
    };

    template <typename MetadataType>
    Metadata make_metadata(MetadataType metadata) {
        return Metadata(typeid(MetadataType), std::move(metadata));
    }

    using PhantomData = std::shared_ptr<void>;

    class PhantomDataProvider {
    public:
        virtual ~PhantomDataProvider() = default;

        [[nodiscard]] virtual PhantomData phantom() const = 0;
    };

    /**
     * A struct to represent a return value of a method.
     * @note This class does not only serve as a proxy,
     * @note but also handles the @b life @b cycle of the actual return value.
     * @note When the ReturnValueProxy is destroyed,
     * @note the actual return value @b WILL @b BE @b DESTROYED as well!
     * @note The discard may not be so obvious,
     * @note especially when using an identifier to store an instance of ReturnValueProxy,
     * @note extracted the raw pointer and then reused the identifier for another instance.
     * @note Here the first ReturnValueProxy, along with the actual data, is invalidated!
     * @note See PhantomDataHelper for more details.
     * @code
     * auto proxy = reflection.invoke_function("ctor", args1);
     * void* ptr = proxy.get_raw();
     * proxy = reflection.invoke_function("ctor", args2); // the 'ptr' is invalidated here.
     * do_something(ptr); // undefined behavior!
     * @endcode
     */
    class ReturnValueProxy : public PhantomDataProvider {
        /**
         * pointer to the return value.
         * @note this does not only serve as a pointer,
         * @note but also handles the @b life @b cycle of the actual return value.
         * @note so when the ReturnValueProxy is destroyed,
         * @note the actual return value WILL BE DESTROYED as well!
         */
        std::shared_ptr<void> ptr;
        size_t size;
        std::type_index type_index = typeid(void);

    public:
        template <typename ValueType>
        explicit ReturnValueProxy(ValueType&& ptr): type_index(typeid(ValueType)) {
            this->ptr = std::make_shared<ValueType>(std::forward<ValueType>(ptr));
            this->size = sizeof(ValueType);
        }

        ReturnValueProxy(std::shared_ptr<void> ptr, size_t size, std::type_index type_index) {
            this->ptr = std::move(ptr);
            this->size = size;
            this->type_index = type_index;
        }

        template <typename ValueType>
        ValueType get() {
            return *static_cast<ValueType *>(this->ptr.get());
        }

        /**
         * Get the raw pointer to the return value.
         * Be aware that the life cycle of the actual return value can be unclear if you use this method.
         * @return the raw pointer to the return value.
         */
        [[nodiscard]] void* get_raw() const {
            return this->ptr.get();
        }

        [[nodiscard]] size_t get_size() const {
            return this->size;
        }

        /**
         * Alias for get_ptr().
         * @return a copy of the shared pointer to the return value.
         */
        [[nodiscard]] std::shared_ptr<void> duplicate_inner() const {
            return get_ptr();
        }

        /**
         * Get a copy of the shared pointer to the return value.
         * @return a copy of the shared pointer to the return value.
         */
        [[nodiscard]] std::shared_ptr<void> get_ptr() const {
            return this->ptr;
        }

        ReturnValueProxy(const ReturnValueProxy& other) {
            this->ptr = other.ptr;
            this->size = other.size;
            this->type_index = other.type_index;
        }

        /**
         * Get a copy of the ReturnValueProxy.
         * @return a copy of the ReturnValueProxy.
         */
        [[nodiscard]] ReturnValueProxy duplicate() const {
            return {this->ptr, this->size, this->type_index};
        }

        [[nodiscard]] std::type_index get_type_index() const {
            return this->type_index;
        }

        /**
         * Convert the ReturnValueProxy to a RawObjectWrapper.
         * @return a RawObjectWrapper.
         */
        [[nodiscard]] RawObjectWrapper to_wrapped() const {
            return {this->ptr.get(), this->type_index};
        }

        [[nodiscard]] SharedObjectWrapper to_shared() const {
            return {this->ptr, this->type_index};
        }

        [[nodiscard]] std::shared_ptr<void> phantom() const override {
            return this->ptr;
        }
    };

    /**
     * A struct to represent a phantom data.
     * This class is mainly applied to handle the life cycle of classes like ReturnValueProxy.
     * In cases like reusing the same identifier to store different instances of ReturnValueProxy,
     * while keeping the memory valid.
     */
    class PhantomDataHelper {
        std::stack<PhantomData> phantom_data = {};

    public:
        PhantomDataHelper() {
            this->phantom_data = {};
        }

        ~PhantomDataHelper() {
            while (!this->phantom_data.empty()) {
                this->phantom_data.pop();
            }
        }

        PhantomDataHelper(const PhantomDataHelper& other) {
            this->phantom_data = other.phantom_data;
        }

        PhantomDataHelper(PhantomDataHelper&& other) noexcept {
            this->phantom_data = std::move(other.phantom_data);
        }

        PhantomDataHelper& operator=(const PhantomDataHelper& other) = default;

        PhantomDataHelper& operator=(PhantomDataHelper&& other) noexcept {
            this->phantom_data = std::move(other.phantom_data);
            return *this;
        }

        void push(PhantomData phantom) {
            this->phantom_data.push(std::move(phantom));
        }

        void clear() {
            while (!this->phantom_data.empty()) {
                this->phantom_data.pop();
            }
        }

        PhantomDataHelper& operator<<(PhantomData rhs) {
            this->push(std::move(rhs));
            return *this;
        }

        PhantomDataHelper& operator<<(const PhantomDataProvider& rhs) {
            this->push(std::move(rhs.phantom()));
            return *this;
        }

        friend PhantomDataHelper& operator>>(PhantomData lhs, PhantomDataHelper& rhs) {
            rhs.push(std::move(lhs));
            return rhs;
        }

        friend PhantomDataHelper& operator>>(const PhantomDataProvider& lhs, PhantomDataHelper& rhs) {
            rhs.push(std::move(lhs.phantom()));
            return rhs;
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
                constexpr size_t arg_size = sizeof...(ArgTypes);
                if constexpr (std::is_void_v<ReturnType>) {
                    if constexpr (arg_size == 0) {
                        (cls->*method)();
                        return ReturnValueProxy(0);
                    }
                    (cls->*method)(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(*(args + Indices)))...);
                    return ReturnValueProxy(0);
                } else {
                    if constexpr (arg_size == 0) {
                        return ReturnValueProxy((cls->*method)());
                    }
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
                constexpr size_t arg_size = sizeof...(ArgTypes);
                if constexpr (std::is_void_v<ReturnType>) {
                    if constexpr (arg_size == 0) {
                        function();
                        return ReturnValueProxy(0);
                    }
                    function(
                        std::forward<remove_cvref_t<ArgTypes>>(
                            *reinterpret_cast<remove_cvref_t<ArgTypes> *>(*(args + Indices)))...);
                    return ReturnValueProxy(0);
                } else {
                    if constexpr (arg_size == 0) {
                        auto ret = function();
                        return ReturnValueProxy(std::move(ret));
                    }
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

    ReflectionBase& get_reflection(std::type_index index);

    using NameTypeInfo = std::pair<std::string, std::type_index>;

    using NameTypeInfoList = std::vector<NameTypeInfo>;
    using NameTypeInfoMap = std::unordered_map<std::string, NameTypeInfo>;

    // Return type, class type, argument types.
    using MethodInfo = std::tuple<std::type_index, std::type_index, std::vector<std::type_index>>;
    // Return type, argument types.
    using FunctionInfo = std::tuple<std::type_index, std::vector<std::type_index>>;

    using CallableInfo = std::variant<MethodInfo, FunctionInfo>;
    using OverloadedCallableInfo = std::vector<CallableInfo>;

    using NameCallableInfo = std::pair<std::string, OverloadedCallableInfo>;

    using NameCallableInfoList = std::vector<NameCallableInfo>;
    using NameCallableInfoMap = std::unordered_map<std::string, NameCallableInfo>;

    /**
     * A class for reflection.
     */
    class ReflectionBase {
        std::unordered_map<std::string, Member> m_offsets = {};
        std::unordered_map<std::string, std::pmr::vector<CallableWrapper>> m_funcs = {};
        std::unordered_map<std::string, Metadata> m_metadata = {};

        std::pmr::vector<std::type_index> m_derived_from;

        std::type_index m_base_type_index = typeid(void);
        std::string m_base_type_name;
        ParsedTypeString m_type_parsed;

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

        static CallableInfo _parse_callable(const CallableWrapper& func) {
            if (std::holds_alternative<std::monostate>(func.parent_type)) {
                FunctionInfo fn_info = std::make_tuple(
                    func.return_type, std::vector<std::type_index>{
                        func.arg_types.begin(),
                        func.arg_types.end()
                    });
                return {fn_info};
            }
            MethodInfo fn_info = std::make_tuple(
                func.return_type, std::get<std::type_index>(func.parent_type),
                std::vector<std::type_index>{func.arg_types.begin(), func.arg_types.end()});
            return {fn_info};
        }

        template <typename ReturnType, typename... ArgTypes>
        ReturnType _propagate(ReflectionBase& base,
                              ReturnType (ReflectionBase::*method)(ArgTypes...),
                              ArgTypes&&... args) {
            return (base.*method)(std::forward<ArgTypes>(args)...);
        }

    public:
        ReflectionBase() = delete;

        explicit ReflectionBase(std::type_index base_type_index, std::string&& base_type_name)
            : m_base_type_index(base_type_index) {
            m_base_type_name = std::move(base_type_name);
            m_type_parsed = parse_type_string(m_base_type_name);
        }

        std::type_index get_type() const {
            return m_base_type_index;
        }

        std::string get_type_string() const {
            return m_base_type_name;
        }

        ParsedTypeString get_type_parsed() const {
            return m_type_parsed;
        }

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
            m_offsets.emplace(std::move(name),
                              Member(offset, sizeof(MemberType), is_const, typeid(MemberType))
                              .init_setter<MemberType>());

            return *this;
        }

        ReflectionBase& derives_from(std::type_index parent) {
            m_derived_from.emplace_back(parent);
            return *this;
        }

        template <typename Parent>
        ReflectionBase& derives_from() {
            m_derived_from.emplace_back(typeid(Parent));
            return *this;
        }

        NameTypeInfoList get_member_list() {
            NameTypeInfoList list;
            for (const auto& [name, member]: m_offsets) {
                list.emplace_back(name, member.type_info);
            }
            return list;
        }

        NameTypeInfoMap get_member_map() {
            NameTypeInfoMap map;
            for (const auto& [name, member]: m_offsets) {
                map.emplace(name, NameTypeInfo(name, member.type_info));
            }
            return map;
        }

        NameCallableInfoList get_callable_list() {
            NameCallableInfoList list;
            for (const auto& [name, callable]: m_funcs) {
                OverloadedCallableInfo info;
                for (const auto& func: callable) {
                    info.emplace_back(_parse_callable(func));
                }
                list.emplace_back(name, std::move(info));
            }
            return list;
        }

        NameCallableInfoMap get_callable_map() {
            NameCallableInfoMap map;
            for (const auto& [name, callable]: m_funcs) {
                OverloadedCallableInfo info;
                for (const auto& func: callable) {
                    info.emplace_back(_parse_callable(func));
                }
                map.emplace(name, NameCallableInfo(name, std::move(info)));
            }
            return map;
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
        template <
            typename MemberType, typename ClassType,
            std::enable_if_t<!std::is_pointer_v<ClassType>, bool>  = false
        >
        MemberType* get_member_ref(ClassType& object, std::string&& name) noexcept {
            return get_member_ref<MemberType, ClassType>(&object, std::move(name));
        }

        template <
            typename MemberType, typename ClassType,
            std::enable_if_t<!std::is_pointer_v<ClassType>, bool>  = false
        >
        MemberType* get_member_ref(ClassType& object, std::string& name) noexcept {
            return get_member_ref<MemberType, ClassType>(&object, name);
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
            return get_const_member_ref<MemberType, ClassType>(&object, name);
        }

        template <typename MemberType, typename ClassType>
        const MemberType* get_const_member_ref(ClassType& object, std::string& name) noexcept {
            return get_const_member_ref<MemberType, ClassType>(&object, name);
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

            for (auto base: m_derived_from) {
                auto reflection = get_reflection(base);
                if (auto propagated = _propagate<MemberType*, ClassType*, std::string&&>(
                        reflection,
                        get_member_ref<MemberType, ClassType>, std::move(object), std::move(std::string(name)));
                    propagated != nullptr) {
                    return propagated;
                }
            }
            return nullptr;
        }

        template <typename MemberType, typename ClassType>
        MemberType* get_member_ref(ClassType* object, std::string& name) noexcept {
            return get_member_ref<MemberType, ClassType>(object, std::move(name));
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

            for (auto base: m_derived_from) {
                auto reflection = get_reflection(base);
                if (auto propagated = _propagate<void *, void *, std::string&&>(reflection,
                        get_member_ref, std::move(object), std::move(std::string(name)));
                    propagated != nullptr) {
                    return propagated;
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
        const MemberType* get_const_member_ref(ClassType* object, std::string& name) noexcept {
            using type = typename remove_const<MemberType>::type;
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                if (member.type_info != typeid(type)) {
                    return nullptr;
                }
                return static_cast<const type *>(reinterpret_cast<void *>(object) + member.offset);
            }

            for (auto base: m_derived_from) {
                auto reflection = get_reflection(base);
                if (auto propagated = _propagate(
                    reflection,
                    get_const_member_ref<MemberType, ClassType>, object, name); propagated != nullptr) {
                    return propagated;
                }
            }

            return nullptr;
        }

        /**
         * Provides direct access to the desired const member of a class, with no type safety guarantees.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @return The pointer to the member.
         */
        const void* get_const_member_ref(const void* object, const std::string& name) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                return object + member.offset;
            }

            for (auto base: m_derived_from) {
                auto reflection = get_reflection(base);
                if (auto propagated = _propagate<const void *, const void *, const std::string&>(reflection,
                    get_const_member_ref, std::move(object), name); propagated != nullptr) {
                    return propagated;
                }
            }

            return nullptr;
        }

        RawObjectWrapper get_member_wrapped(void* object, const std::string& name) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                return RawObjectWrapper(object + member.offset, member.type_info);
            }

            for (auto base: m_derived_from) {
                auto reflection = get_reflection(base);
                if (auto propagated =
                        _propagate<RawObjectWrapper, void *, const std::string&>(reflection,
                            get_member_wrapped, std::move(object), name); !propagated.is_none_type()) {
                    return propagated;
                }
            }

            return RawObjectWrapper::none();
        }

        /**
         * Set the value of a member.
         * @note This method includes direct operation on memory, which can be dangerous.
         * @note Even if you are sure that the type matches,
         * @note there's still a chance that undefined behavior occurs.
         * @param object The pointer to the object.
         * @param name The name of the member.
         * @param value The pointer to the value you want to assign to the member.
         */
        void set_member(void* object, const std::string& name, void* value) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                member.setter(object, value);
                return;
            }
            throw std::runtime_error("Member not found");
        }

        template <typename WrapperType, std::enable_if_t<std::is_same_v<remove_cvref_t<WrapperType>, RawObjectWrapper>,
            bool>  = false>
        void set_member(void* object, std::string&& name, WrapperType value) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                if (value.type_index != member.type_info) {
                    throw std::runtime_error("Type mismatch");
                }
                member.setter(object, value.object);
                return;
            }
            throw std::runtime_error("Member not found");
        }

        template <typename WrapperType, std::enable_if_t<std::is_same_v<remove_cvref_t<WrapperType>, RawObjectWrapper>,
            bool>  = false>
        void set_member(void* object, std::string& name, WrapperType value) {
            if (const auto find = m_offsets.find(name); find != m_offsets.end()) {
                const auto& member = find->second;
                if (value.type_index != member.type_info) {
                    throw std::runtime_error("Type mismatch");
                }
                member.setter(object, value.object);
                return;
            }
            throw std::runtime_error("Member not found");
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
            std::enable_if_t<std::is_void_v<ReturnType>, bool>  = false,
            std::enable_if_t<(sizeof ...(ArgTypes) > 0), bool>  = false
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
        template <typename ReturnType, std::enable_if_t<!std::is_void_v<ReturnType>, bool>  = false>
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
        template <typename ReturnType, std::enable_if_t<std::is_void_v<ReturnType>, bool>  = false>
        ReturnType invoke_method(void* object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (can_cast_to<std::function<void(void*)>>(fn.method)) {
                        const auto method = std::any_cast<std::function<void(void*)>>(fn.method);
                        method(object);
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

        ReturnValueProxy invoke_function(std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    if (fn.arg_types.empty()) {
                        ReturnValueProxy proxy = fn.callable(nullptr, nullptr);
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
        ReturnValueProxy invoke_method(ClassType& object, std::string&& name, const ArgList& args) {
            try {
                return invoke_method(&object, std::move(name), std::move(args));
            } catch (const method_not_found_exception&) {
                throw method_not_found_exception(name);
            }
        }

        template <typename ClassType, std::enable_if_t<!std::is_void_v<ClassType>, bool>  = false>
        ReturnValueProxy invoke_method(ClassType* object, std::string&& name, const ArgList& args) {
            try {
                return invoke_method(static_cast<void *>(object), std::move(name), args);
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

        template <typename ClassType>
        ReturnValueProxy invoke_method(ClassType* object, std::string&& name) {
            if (const auto find = m_funcs.find(name); find != m_funcs.end()) {
                auto fn_overloads = find->second;
                for (auto& fn: fn_overloads) {
                    ReturnValueProxy proxy = fn.callable(object, nullptr);
                    return proxy;
                }
            }
            throw method_not_found_exception(name);
        }

        ReflectionBase& attach_metadata(std::string name, Metadata metadata) {
            m_metadata.emplace(std::move(name), std::move(metadata));
            return *this;
        }

        template <typename MetadataType>
        ReflectionBase& attach_metadata(std::string name, MetadataType metadata) {
            if constexpr (std::is_convertible_v<MetadataType, std::string>) {
                m_metadata.emplace(std::move(name), make_metadata(std::move(std::string(metadata))));
                return *this;
            }
            m_metadata.emplace(std::move(name), make_metadata(std::move(metadata)));
            return *this;
        }

        Metadata get_metadata(const std::string& name) {
            if (const auto find = m_metadata.find(name); find != m_metadata.end()) {
                return find->second;
            }
            throw metadata_not_found_exception(name);
        }

        template <typename MetadataType>
        MetadataType get_metadata_as(const std::string& name) {
            if (const auto find = m_metadata.find(name); find != m_metadata.end()) {
                if (find->second.type_index != typeid(MetadataType)) {
                    throw std::runtime_error("Type mismatch");
                }
                std::any any = find->second.data;
                return std::any_cast<MetadataType>(any);
            }
            throw metadata_not_found_exception(name);
        }

        bool has_metadata(const std::string& name) {
            return m_metadata.find(name) != m_metadata.end();
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

    class reflection_registry_not_found_exception : public std::runtime_error {
    public:
        [[nodiscard]] const char* what() const noexcept override {
            return "ReflectionRegistryBase not found.";
        }

        explicit reflection_registry_not_found_exception(const std::string& what) : std::runtime_error(what) {
            std::cerr << what << std::endl;
        }
    };

    class ReflectionRegistryBase {
        std::pmr::unordered_map<std::type_index, ReflectionBase> m_reflections;
        std::unordered_map<std::string, std::type_index> m_type_index_map = {};

    public:
        ReflectionRegistryBase() = default;

        ReflectionRegistryBase(const ReflectionRegistryBase&) = delete;

        ReflectionRegistryBase(ReflectionRegistryBase&&) = delete;

        static ReflectionRegistryBase& instance() {
            static ReflectionRegistryBase _instance;
            return _instance;
        }

        ReflectionBase& register_base(std::type_index type_index, ReflectionBase reflection) {
            m_reflections.insert({type_index, std::move(reflection)});
            return m_reflections.at(type_index);
        }

        template <typename ClassType>
        ReflectionBase& register_base() {
            m_type_index_map.emplace(extract_type_name<ClassType>(), typeid(ClassType));
            return register_base(typeid(ClassType), ReflectionBase(typeid(ClassType),
                                                                   extract_type_name<ClassType>()));
        }

        ReflectionBase& get_reflection(const std::string& type_name) {
            try {
                return m_reflections.at(m_type_index_map.at(type_name));
            } catch (const std::out_of_range&) {
                std::stringstream ss;
                ss << "ReflectionRegistryBase not found for type with name. " <<
                        "Perhaps you forgot to register it, " <<
                        "or you did not register it with the override which supports this function: " <<
                        type_name;
                throw reflection_registry_not_found_exception(ss.str());
            }
        }

        template <typename ClassType>
        ReflectionBase& get_reflection() {
            try {
                return m_reflections.at(typeid(ClassType));
            } catch (const std::out_of_range&) {
                std::stringstream ss;
                ss << "ReflectionRegistryBase not found for type with typeid: " << typeid(ClassType).name();
                throw reflection_registry_not_found_exception(ss.str());
            }
        }

        ReflectionBase& get_reflection(const std::type_index type_index) {
            try {
                return m_reflections.at(type_index);
            } catch (const std::out_of_range&) {
                std::stringstream ss;
                ss << "ReflectionRegistryBase not found for type with typeid: " << type_index.name();
                throw reflection_registry_not_found_exception(ss.str());
            }
        }
    };

    [[deprecated]] inline ReflectionBase& make_reflection(const std::type_index type_index) {
        return ReflectionRegistryBase::instance().register_base(type_index, ReflectionBase(type_index, "__NULL__"));
    }

    template <typename ClassType>
    ReflectionBase& make_reflection() {
        return ReflectionRegistryBase::instance().register_base<ClassType>();
    }

    inline ReflectionBase& get_reflection(std::type_index index) {
        return ReflectionRegistryBase::instance().get_reflection(index);
    }
}

#endif //SIMPLE_REFL_H
