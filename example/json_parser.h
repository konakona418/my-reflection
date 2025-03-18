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
            return c == ' ' || c == '\t';
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
            for (const char& it : str) {
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

    inline void print_object(JsonObject object) {
        switch (object.value.index()) {
            case 0:
                std::cout << "null";
                break;
            case 1:
                std::cout << "\"";
                std::cout << _internal::back_convert_escape_char(std::get<std::string>(object.value));
                std::cout << "\"";
                break;
            case 2:
                std::cout << std::get<int>(object.value);
                break;
            case 3:
                std::cout << std::get<double>(object.value);
                break;
            case 4:
                std::cout << (std::get<bool>(object.value) ? "true" : "false");
                break;
            case 5:
                do {
                    std::cout << "[";
                    size_t i = 0;
                    size_t size = std::get<JsonArray>(object.value).size();
                    for (const auto& item: std::get<JsonArray>(object.value)) {
                        print_object(item);
                        if (++i != size) {
                            std::cout << ",";
                        }
                    }
                    std::cout << "]";
                } while (false);
                break;
            case 6:
                do {
                    size_t i = 0;
                    size_t size = std::get<JsonMap>(object.value).size();
                    std::cout << "{";
                    for (const auto& [key, value]: std::get<JsonMap>(object.value)) {
                        std::cout << "\"" << key << "\"" << ":";
                        print_object(value);
                        if (++i != size) {
                            std::cout << ",";
                        }
                    }
                    std::cout << "}";
                } while (false);
                break;
            default:
                std::cout << "unknown";
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

inline simple_reflection::PhantomDataHelper phantom;
namespace json_mapper {

    inline bool is_json_primitives(std::type_index type_index) {
        return type_index == typeid(std::string) ||
               type_index == typeid(int) ||
               type_index == typeid(double) ||
               type_index == typeid(bool) ||
               type_index == typeid(std::monostate);
    }

    inline simple_reflection::ReturnValueProxy map_fields(simple_reflection::ReflectionBase& reflection,
                                                          const json_parser::JsonMap& map) {
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

            simple_reflection::ReflectionBase field_reflection =
                simple_reflection::ReflectionRegistryBase::instance().get_reflection(field_type);
            auto proxy = map_fields(field_reflection, std::get<json_parser::JsonMap>(value.value));
            reflection.set_member(instance_ptr, field_name, proxy.to_wrapped());
        }
        return instance;
    }

    template <typename Serializable>
    simple_reflection::ReturnValueProxy from_json(const std::string& json_str) {
        auto json_object = json_parser::parse_json_object(json_str);
        auto base = simple_reflection::ReflectionRegistryBase::instance().get_reflection(typeid(Serializable));
        auto instance = map_fields(base,
                                   std::get<json_parser::JsonMap>(json_object.value));
        return instance;
    }
}

namespace json_parser_test {
    class TestInternal {
    public:
        std::string str;
        int num;

        TestInternal(): num(0) {}

        void print() const {
            std::cout << "(Internal)" << "str: " << str << ", num: " << num << std::endl;
        }
    };

    static auto test_internal_refl = simple_reflection::make_reflection<TestInternal>()
        .register_member<&TestInternal::str>("str")
        .register_member<&TestInternal::num>("num")
        .register_function<TestInternal>("ctor", []() { return TestInternal(); });

    class Test {
    public:
        std::string name;
        int age;
        double height;
        bool gender;

        TestInternal internal = TestInternal();

        Test(): age(0), height(0), gender(false) {}

        void print() {
            std::cout << "name: " << name << ", age: " << age << ", height: " << height << ", gender: " << gender << std::endl;
            internal.print();
        }
    };

    static auto test_refl = simple_reflection::make_reflection<Test>()
        .register_member<&Test::name>("name")
        .register_member<&Test::age>("age")
        .register_member<&Test::height>("height")
        .register_member<&Test::gender>("gender")
        .register_member<&Test::internal>("internal")
        .register_function<Test>("ctor", []() { return Test(); });

    inline void test_parse_json() {
        std::string json_str;
        std::getline(std::cin, json_str);
        auto result = json_parser::parse_json_object(json_str);
        auto proxy = json_mapper::from_json<Test>(json_str);

        auto deserialized = proxy.to_wrapped().deref_into<Test>();
        deserialized.print();
        // print_object(result);
    }
}

#endif //JSON_PARSER_H
