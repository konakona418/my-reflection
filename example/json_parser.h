//
// Created on 2025/3/18.
//

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stack>
#include <string>

#include <simple_refl.h>

namespace json_parser {
    struct JsonObject;

    using JsonArray = std::vector<JsonObject>;
    using JsonMap = std::unordered_map<std::string, JsonObject>;

    struct JsonObject {
        std::variant<
            std::monostate,
            std::string,
            int,
            double,
            bool,
            JsonArray,
            JsonMap
        > value;

        bool operator==(const JsonObject& rhs) const {
            return value == rhs.value;
        }

        bool operator!=(const JsonObject& rhs) const {
            return value != rhs.value;
        }
    };

    namespace _internal {
        inline bool is_escape_char(char c) {
            return c == '\\' || c == '\"' || c == '\'' || c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t';
        }

        inline bool is_empty_char(char c) {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        inline char convert_escape_char(char c) {
            switch (c) {
                case '\\':
                    return '\\';
                case '\"':
                    return '\"';
                case '\'':
                    return '\'';
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                default:
                    return c;
            }
        }

        inline std::string back_convert_escape_char(char c) {
            switch (c) {
                case '\\':
                    return "\\\\";
                case '\"':
                    return "\\\"";
                case '\'':
                    return "\\\'";
                case '\b':
                    return "\\b";
                case '\f':
                    return "\\f";
                case '\n':
                    return "\\n";
                case '\r':
                    return "\\r";
                case '\t':
                    return "\\t";
                default:
                    return {1, c};
            }
        }

        inline std::string back_convert_escape_char(const std::string& str) {
            std::string result;
            result.reserve(str.size());
            for (const auto& c: str) {
                if (is_escape_char(c)) {
                    result += back_convert_escape_char(c);
                    continue;
                }
                result += c;
            }
            return result;
        }

        inline void skip_empty(std::string::const_iterator& it) {
            while (is_empty_char(*it)) {
                ++it;
            }
        }

        inline void to_nearest_colon(std::string::const_iterator& it) {
            while (*it != ':') {
                ++it;
            }
            ++it;
        }

        inline bool is_left_bracket(char c) {
            return c == '[' || c == '{' || c == '(';
        }

        inline bool is_right_bracket(char c) {
            return c == ']' || c == '}' || c == ')';
        }

        inline bool bracket_match(char left, char right) {
            return (left == '[' && right == ']') || (left == '{' && right == '}') || (left == '(' && right == ')');
        }

        inline bool check_brackets(const std::string& str) {
            std::stack<char> brackets;
            for (const char& it: str) {
                if (is_left_bracket(it)) {
                    brackets.push(it);
                } else if (is_right_bracket(it)) {
                    if (brackets.empty()) {
                        throw std::runtime_error("brackets error");
                    }
                    if (bracket_match(brackets.top(), it)) {
                        brackets.pop();
                    } else {
                        return false;
                    }
                }
            }
            while (!brackets.empty()) {
                return false;
            }
            return true;
        }
    }

    inline void print_object(JsonObject object, std::ostream& os = std::cout, bool pretty_print = false, size_t indent = 4) {
        switch (object.value.index()) {
            case 0:
                os << "null";
                break;
            case 1:
                os << "\"";
                os << _internal::back_convert_escape_char(std::get<std::string>(object.value));
                os << "\"";
                break;
            case 2:
                os << std::get<int>(object.value);
                break;
            case 3:
                os << std::get<double>(object.value);
                break;
            case 4:
                os << (std::get<bool>(object.value) ? "true" : "false");
                break;
            case 5:
                do {
                    os << "[";
                    if (pretty_print) {
                        os << std::endl;
                    }
                    size_t i = 0;
                    size_t size = std::get<JsonArray>(object.value).size();
                    for (const auto& item: std::get<JsonArray>(object.value)) {
                        if (pretty_print) {
                            os << std::string(indent, ' ');
                        }
                        print_object(item, os, pretty_print, indent + 4);
                        if (++i != size) {
                            os << ", ";
                        }
                        if (pretty_print) {
                            os << std::endl;
                        }
                    }
                    if (pretty_print) {
                        os << std::string(indent - 4, ' ');
                    }
                    os << "]";
                } while (false);
                break;
            case 6:
                do {
                    size_t i = 0;
                    size_t size = std::get<JsonMap>(object.value).size();
                    os << "{";
                    if (pretty_print) {
                        os << std::endl;
                    }
                    for (const auto& [key, value]: std::get<JsonMap>(object.value)) {
                        if (pretty_print) {
                            os << std::string(indent, ' ');
                        }
                        os << "\"" << key << "\"" << ": ";
                        print_object(value, os, pretty_print, indent + 4);
                        if (++i != size) {
                            os << ", ";
                        }
                        if (pretty_print) {
                            os << std::endl;
                        }
                    }
                    if (pretty_print) {
                        os << std::string(indent - 4, ' ');
                    }
                    os << "}";
                } while (false);
                break;
            default:
                os << "unknown";
        }
    }

    JsonArray parse_json_array(std::string::const_iterator& it);

    JsonMap parse_json_map(std::string::const_iterator& it);

    std::string parse_json_string(std::string::const_iterator& it);

    inline JsonObject parse_json_object(std::string::const_iterator& it) {
        _internal::skip_empty(it);
        if (*it == '[') {
            return {parse_json_array(it)};
        }
        if (*it == '{') {
            return {parse_json_map(it)};
        }
        if (*it == '"') {
            return {parse_json_string(it)};
        }
        if (*it == 't') {
            it += 3;
            return {true};
        }
        if (*it == 'f') {
            it += 3;
            return {false};
        }
        if (*it == 'n') {
            it += 3;
            return {std::monostate()};
        }
        if (*it == '-' || (*it >= '0' && *it <= '9')) {
            std::string number;
            int sgn = *it == '-' ? -1 : 1;
            if (*it == '-') {
                ++it;
            }
            while ((*it >= '0' && *it <= '9') || *it == '.') {
                number += *it;
                ++it;
            }
            --it;
            if (number.find('.') != std::string::npos) {
                return {sgn * std::stod(number)};
            }
            return {sgn * std::stoi(number)};
        }
        throw std::runtime_error("unknown json object");
    }

    inline JsonArray parse_json_array(std::string::const_iterator& it) {
        JsonArray result;
        while (*++it != ']') {
            _internal::skip_empty(it);
            if (*it == ',') {
                ++it;
            }
            _internal::skip_empty(it);
            if (*it == ']') {
                return result;
            }
            JsonObject object = {parse_json_object(it)};
            result.push_back(object);
        }
        return result;
    }

    inline std::string parse_json_string(std::string::const_iterator& it) {
        std::string result;
        result.reserve(64);
        while (*++it != '"') {
            if (*it == '\\') {
                ++it;
                result.push_back(_internal::convert_escape_char(*it));
                continue;
            }
            result.push_back(*it);
        }
        return result;
    }

    inline JsonMap parse_json_map(std::string::const_iterator& it) {
        JsonMap result;
        while (*++it != '}') {
            _internal::skip_empty(it);
            if (*it == ',') {
                ++it;
            }
            if (*it == '}') {
                return result;
            }
            if (*it == '"') {
                std::string key = parse_json_string(it);
                _internal::skip_empty(it);
                _internal::to_nearest_colon(it);
                result[key] = {parse_json_object(it)};
            }
        }
        return result;
    }

    inline JsonObject parse_json_object(const std::string& json_str) {
        JsonObject result;
        if (json_str.empty()) {
            result.value = std::monostate();
            return result;
        }

        if (!_internal::check_brackets(json_str)) {
            throw std::runtime_error("brackets are not balanced");
        }

        auto it = json_str.begin();
        _internal::skip_empty(it);
        result = parse_json_object(it);
        return result;
    }
}

namespace json_mapper {
    template <typename T>
    class JsonVector : public std::vector<T> {
    public:
        std::type_index type_index = typeid(T);

        JsonVector() = default;

        void push_back(T&& value) {
            std::vector<T>::push_back(std::move(value));
        }

        T pop_back() {
            auto val = std::vector<T>::at(std::vector<T>::size() - 1);
            std::vector<T>::pop_back();
            return val;
        }

        size_t size() {
            return std::vector<T>::size();
        }
    };

    inline bool is_json_primitives(std::type_index type_index) {
        return type_index == typeid(std::string) ||
               type_index == typeid(int) ||
               type_index == typeid(double) ||
               type_index == typeid(bool) ||
               type_index == typeid(std::monostate);
    }

    simple_reflection::ReturnValueProxy map_fields(simple_reflection::ReflectionBase& reflection,
                                                   const json_parser::JsonMap& map,
                                                   simple_reflection::PhantomDataHelper& phantom);

    inline simple_reflection::ReturnValueProxy map_array(simple_reflection::ReflectionBase& reflection,
                                                         const json_parser::JsonArray& array,
                                                         simple_reflection::PhantomDataHelper& phantom) {
        auto instance = reflection.invoke_function("ctor");
        instance >> phantom;
        auto array_ptr = instance.get_raw();

        auto elem_type = *reflection.get_member_ref<std::type_index>(array_ptr, "type_index");
        for (const auto& item: array) {
            if (is_json_primitives(elem_type)) {
                if (elem_type == typeid(std::string)) {
                    reflection.invoke_method(array_ptr, "push_back",
                                             make_args(const_cast<std::string&&>(std::get<std::string>(item.value))));
                    continue;
                }
                if (elem_type == typeid(int)) {
                    reflection.invoke_method(array_ptr, "push_back",
                                             make_args(const_cast<int&&>(std::get<int>(item.value))));
                    continue;
                }
                if (elem_type == typeid(double)) {
                    reflection.invoke_method(array_ptr, "push_back",
                                             make_args(const_cast<double&&>(std::get<double>(item.value))));
                    continue;
                }
                if (elem_type == typeid(bool)) {
                    reflection.invoke_method(array_ptr, "push_back",
                                             make_args(const_cast<bool&&>(std::get<bool>(item.value))));
                    continue;
                }
                if (elem_type == typeid(std::monostate)) {
                    throw std::runtime_error("nullable type is not supported");
                }
            }

            auto item_refl = simple_reflection::ReflectionRegistryBase::instance().get_reflection(elem_type);
            auto proxy = map_fields(item_refl, std::get<json_parser::JsonMap>(item.value), phantom);
            proxy >> phantom;
            reflection.invoke_method(array_ptr, "push_back",
                                     simple_reflection::empty_arg_list() | proxy.to_wrapped());
        }

        return instance;
    }

    inline simple_reflection::ReturnValueProxy map_fields(simple_reflection::ReflectionBase& reflection,
                                                          const json_parser::JsonMap& map,
                                                          simple_reflection::PhantomDataHelper& phantom) {
        auto instance = reflection.invoke_function("ctor");
        instance >> phantom;
        auto instance_ptr = instance.get_raw();

        auto fields = reflection.get_member_map();

        for (const auto& [key, value]: map) {
            if (fields.find(key) == fields.end()) {
                continue;
            }
            auto& [field_name, field_type] = fields.at(key);
            if (is_json_primitives(field_type)) {
                if (field_type == typeid(std::string)) {
                    const auto field_ptr = reflection.get_member_ref<std::string>(instance_ptr, field_name);
                    *field_ptr = std::get<std::string>(value.value);
                    continue;
                }
                if (field_type == typeid(int)) {
                    const auto field_ptr = reflection.get_member_ref<int>(instance_ptr, field_name);
                    *field_ptr = std::get<int>(value.value);
                    continue;
                }
                if (field_type == typeid(double)) {
                    const auto field_ptr = reflection.get_member_ref<double>(instance_ptr, field_name);
                    *field_ptr = std::get<double>(value.value);
                    continue;
                }
                if (field_type == typeid(bool)) {
                    const auto field_ptr = reflection.get_member_ref<bool>(instance_ptr, field_name);
                    *field_ptr = std::get<bool>(value.value);
                    continue;
                }
                if (field_type == typeid(std::monostate)) {
                    throw std::runtime_error("nullable field is not supported");
                }
            }

            simple_reflection::PhantomDataHelper field_phantom;

            if (value.value.index() == 5) {
                auto array_refl = simple_reflection::ReflectionRegistryBase::instance().get_reflection(field_type);
                auto proxy = map_array(array_refl, std::get<json_parser::JsonArray>(value.value), field_phantom);
                reflection.set_member(instance_ptr, field_name, proxy.to_wrapped());
                continue;
            }

            simple_reflection::ReflectionBase field_reflection =
                    simple_reflection::ReflectionRegistryBase::instance().get_reflection(field_type);

            auto proxy = map_fields(field_reflection, std::get<json_parser::JsonMap>(value.value), field_phantom);
            reflection.set_member(instance_ptr, field_name, proxy.to_wrapped());
        }
        return instance;
    }

    template <typename Serializable>
    simple_reflection::ReturnValueProxy from_json(const std::string& json_str) {
        simple_reflection::PhantomDataHelper phantom;
        auto json_object = json_parser::parse_json_object(json_str);
        auto base = simple_reflection::ReflectionRegistryBase::instance().get_reflection(typeid(Serializable));
        if (json_object.value.index() == 5) {
            return map_array(base, std::get<json_parser::JsonArray>(json_object.value), phantom);
        }
        return map_fields(base, std::get<json_parser::JsonMap>(json_object.value), phantom);
    }

    json_parser::JsonObject _dump_json_object(void* object, simple_reflection::ReflectionBase& reflection);

    inline json_parser::JsonObject _dump_json_array(void* object, simple_reflection::ReflectionBase& reflection) {
        simple_reflection::PhantomDataHelper phantom;
        auto member_type = *reflection.get_member_ref<std::type_index>(object, "type_index");

        auto proxy = reflection.invoke_method(object, "size");
        auto size = proxy.to_wrapped().deref_into<size_t>();

        json_parser::JsonArray array;

        proxy >> phantom;
        if (is_json_primitives(member_type)) {
            if (member_type == typeid(std::string)) {
                for (size_t i = 0; i < size; ++i) {
                    proxy = reflection.invoke_method(object, "pop_back");
                    proxy >> phantom;
                    array.push_back(json_parser::JsonObject{
                        std::move(proxy.to_wrapped().deref_into<std::string>())
                    });
                }
                return json_parser::JsonObject{std::move(array)};
            }
            if (member_type == typeid(int)) {
                for (size_t i = 0; i < size; ++i) {
                    proxy = reflection.invoke_method(object, "pop_back");
                    proxy >> phantom;
                    array.push_back(json_parser::JsonObject{
                        proxy.to_wrapped().deref_into<int>()
                    });
                }
            }
            if (member_type == typeid(double)) {
                for (size_t i = 0; i < size; ++i) {
                    proxy = reflection.invoke_method(object, "pop_back");
                    proxy >> phantom;
                    array.push_back(json_parser::JsonObject{
                        proxy.to_wrapped().deref_into<double>()
                    });
                }
            }
            if (member_type == typeid(bool)) {
                for (size_t i = 0; i < size; ++i) {
                    proxy = reflection.invoke_method(object, "pop_back");
                    proxy >> phantom;
                    array.push_back(json_parser::JsonObject{
                        proxy.to_wrapped().deref_into<bool>()
                    });
                }
            }
            return json_parser::JsonObject{std::move(array)};
        }

        auto member_refl = simple_reflection::ReflectionRegistryBase::instance().get_reflection(member_type);
        for (size_t i = 0; i < size; ++i) {
            proxy = reflection.invoke_method(object, "pop_back");
            proxy >> phantom;
            auto member_object = _dump_json_object(proxy.get_raw(), member_refl);
            array.push_back(std::move(member_object));
        }
        return json_parser::JsonObject{std::move(array)};
    }

    inline json_parser::JsonObject _dump_json_object(void* object,
                                                     simple_reflection::ReflectionBase& reflection) {
        auto fields = reflection.get_member_map();

        json_parser::JsonMap map;

        for (auto& [_, pair]: fields) {
            auto field_type = pair.second;
            auto name = std::string(pair.first);

            json_parser::JsonObject inner = json_parser::JsonObject();
            if (is_json_primitives(field_type)) {
                if (field_type == typeid(std::string)) {
                    auto member = reflection.get_member_ref<std::string>(object, name);
                    inner.value = *member;
                    map.emplace(std::move(name), std::move(inner));
                    continue;
                }
                if (field_type == typeid(int)) {
                    auto member = reflection.get_member_ref<int>(object, name);
                    inner.value = *member;
                    map.emplace(name, std::move(inner));
                    continue;
                }
                if (field_type == typeid(double)) {
                    auto member = reflection.get_member_ref<double>(object, name);
                    inner.value = *member;
                    map.emplace(name, inner);
                    continue;
                }
                if (field_type == typeid(bool)) {
                    auto member = reflection.get_member_ref<bool>(object, name);
                    inner.value = *member;
                    map.emplace(name, inner);
                    continue;
                }
                if (field_type == typeid(std::monostate)) {
                    throw std::runtime_error("nullable field is not supported");
                }
            }

            try {
                auto field_reflection = simple_reflection::ReflectionRegistryBase::instance().
                        get_reflection(field_type);
                if (field_reflection.has_metadata("json_object_type")) {
                    if (field_reflection.get_metadata_as<std::string>("json_object_type") == "array_like") {
                        auto field_ptr = reflection.get_member_wrapped(object, name);
                        inner = _dump_json_array(field_ptr.object, field_reflection);
                        map.emplace(name, inner);
                        continue;
                    }
                }
                inner = _dump_json_object(object, field_reflection);
                map.emplace(name, inner);
            } catch (const std::exception& e) {
                // std::cout << e.what() << std::endl;
                return json_parser::JsonObject{};
            }
        }
        return json_parser::JsonObject{std::move(map)};
    }

    template <typename Serializable>
    json_parser::JsonObject dump_json_object(Serializable& object) {
        try {
            auto base = simple_reflection::ReflectionRegistryBase::instance().get_reflection(typeid(Serializable));
            return _dump_json_object(&object, base);
        } catch (const std::exception& e) {
            throw std::runtime_error("type " + std::string(typeid(Serializable).name()) + " is not registered");
        }
    }

#define define_json_vector(_Type) \
    static auto _refl_base_##_Type = simple_reflection::make_reflection<json_mapper::JsonVector<_Type>>() \
        .register_method<json_mapper::JsonVector<_Type>, size_t>("size", &json_mapper::JsonVector<_Type>::size) \
        .register_method<json_mapper::JsonVector<_Type>, _Type>("pop_back", &json_mapper::JsonVector<_Type>::pop_back) \
        .register_method<json_mapper::JsonVector<_Type>, void, _Type&&>("push_back",\
            &json_mapper::JsonVector<_Type>::push_back)\
        .register_function<json_mapper::JsonVector<_Type>>("ctor",\
            []() { return json_mapper::JsonVector<_Type>(); })\
        .register_member<&json_mapper::JsonVector<_Type>::type_index>("type_index") \
        .attach_metadata("json_object_type", "array_like")

    using stl_string = std::string;
    define_json_vector(int);
    define_json_vector(double);
    define_json_vector(stl_string);
    define_json_vector(bool);
}

namespace json_parser_test {
    class TestInternal {
    public:
        std::string str;
        int num;

        TestInternal(): num(0) {
        }

        void print() const {
            std::cout << "(Internal)" << "str: " << str << ", num: " << num << std::endl;
        }
    };

    static auto test_internal_refl = simple_reflection::make_reflection<TestInternal>()
            .register_member<&TestInternal::str>("str")
            .register_member<&TestInternal::num>("num")
            .register_function<TestInternal>("ctor", []() { return TestInternal(); });

    class TestListElem {
    public:
        int num;
        std::string str;

        TestListElem(): num(0) {
        }

        void print() const {
            std::cout << "(ListElem)" << "num: " << num << ", str: " << str << std::endl;
        }
    };

    class Test {
    public:
        std::string name;
        int age;
        double height;
        bool gender;

        TestInternal internal = TestInternal();
        json_mapper::JsonVector<int> numbers;
        json_mapper::JsonVector<TestListElem> list;

        Test(): age(0), height(0), gender(false) {
        }

        void print() {
            std::cout << "name: " << name << ", age: " << age << ", height: " << height << ", gender: " << gender <<
                    std::endl;
            std::cout << "internal: " << std::endl;
            internal.print();
            std::cout << "(Array<int>)numbers: ";
            for (const auto& num: numbers) {
                std::cout << num << " ";
            }
            std::cout << std::endl;
            std::cout << "(Array<ListElem>)list: " << std::endl;
            for (const auto& elem: list) {
                elem.print();
            }
        }
    };

    static auto test_list_elem_refl = simple_reflection::make_reflection<TestListElem>()
            .register_member<&TestListElem::num>("num")
            .register_member<&TestListElem::str>("str")
            .register_function<TestListElem>("ctor", []() { return TestListElem(); });

    define_json_vector(TestListElem);

    static auto test_refl = simple_reflection::make_reflection<Test>()
            .register_member<&Test::name>("name")
            .register_member<&Test::age>("age")
            .register_member<&Test::height>("height")
            .register_member<&Test::gender>("gender")
            .register_member<&Test::internal>("internal")
            .register_member<&Test::numbers>("numbers")
            .register_member<&Test::list>("list")
            .register_function<Test>("ctor", []() { return Test(); });

    inline void test_parse_json() {
        std::string json_str = R"({
            "name": "John Smith",
            "age": 30,
            "height": 1.8,
            "gender": true,
            "internal": {
                "str": "Hello",
                "num": 42
            },
            "numbers": [1, 2, 3, 4, 5],
            "list": [
                {
                    "num": 1,
                    "str": "A"
                },
                {
                    "num": 2,
                    "str": "B"
                },
                {
                    "num": 3,
                    "str": "C"
                }
            ]
        })";
        auto result = json_parser::parse_json_object(json_str);
        auto proxy = json_mapper::from_json<Test>(json_str);

        auto deserialized = proxy.to_wrapped().deref_into<Test>();
        deserialized.print();

        print_object(json_mapper::dump_json_object(deserialized), std::cout, true);
        // print_object(result);
    }
}

#endif //JSON_PARSER_H
