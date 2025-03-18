//
// Created on 2025/3/18.
//

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>

#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>

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
                std::cout << "[";
                for (const auto& item: std::get<JsonArray>(object.value)) {
                    print_object(item);
                    std::cout << ",";
                }
                std::cout << "]";
                break;
            case 6:
                std::cout << "{";
                for (const auto& [key, value]: std::get<JsonMap>(object.value)) {
                    std::cout << "\"" << key << "\"" << ":";
                    print_object(value);
                    std::cout << ",";
                }
                std::cout << "}";
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
            return {sgn * std::stod(number)};
        }
        throw std::runtime_error("unknown json object");
    }

    inline JsonArray parse_json_array(std::string::const_iterator& it) {
        JsonArray result;
        while (*++it != ']') {
            if (*it == ',') {
                ++it;
            }
            _internal::skip_empty(it);
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

        auto it = json_str.begin();
        _internal::skip_empty(it);
        result = parse_json_object(it);
        return result;
    }

    inline void test_parse_json() {
        std::string json_str;
        std::getline(std::cin, json_str);
        auto result = parse_json_object(json_str);
        print_object(result);
    }
}

#endif //JSON_PARSER_H
