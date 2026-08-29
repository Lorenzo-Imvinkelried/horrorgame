#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>

namespace json {

enum class Type { Null, Object, Array, String, Number, Boolean };

struct Value;

using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
    Type type = Type::Null;
    std::variant<std::monostate, Object, Array, std::string, double, bool> data;

    Value() = default;
    Value(double d) : type(Type::Number), data(d) {}
    Value(int i) : type(Type::Number), data((double)i) {}
    Value(bool b) : type(Type::Boolean), data(b) {}
    Value(const std::string& s) : type(Type::String), data(s) {}
    Value(const char* s) : type(Type::String), data(std::string(s)) {}
    Value(const Object& o) : type(Type::Object), data(o) {}
    Value(const Array& a) : type(Type::Array), data(a) {}

    double asDouble() const {
        if(auto* v = std::get_if<double>(&data)) return *v;
        return 0.0;
    }
    int asInt() const { return static_cast<int>(asDouble()); }
    std::string asString() const {
        if(auto* v = std::get_if<std::string>(&data)) return *v;
        return "";
    }
    bool asBool() const {
        if(auto* v = std::get_if<bool>(&data)) return *v;
        return false;
    }
    const Object& asObject() const {
        static Object empty;
        if(auto* v = std::get_if<Object>(&data)) return *v;
        return empty;
    }
    const Array& asArray() const {
        static Array empty;
        if(auto* v = std::get_if<Array>(&data)) return *v;
        return empty;
    }
};

class Parser {
    const std::string& str;
    size_t pos = 0;

public:
    Parser(const std::string& s) : str(s) {}

    Value parse() {
        skipWhitespace();
        if (pos >= str.size()) return Value();
        
        char c = str[pos];
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't' || c == 'f') return parseBool();
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        
        return Value();
    }

private:
    void skipWhitespace() {
        while (pos < str.size() && (std::isspace(str[pos]) || str[pos] == '\n' || str[pos] == '\r')) {
            pos++;
        }
    }

    Value parseObject() {
        Object obj;
        pos++; // skip {
        while (true) {
            skipWhitespace();
            if (pos >= str.size() || str[pos] == '}') {
                pos++;
                break;
            }
            // Key
            std::string key = parseString().asString();
            skipWhitespace();
            if (str[pos] == ':') pos++;
            // Value
            obj[key] = parse();
            skipWhitespace();
            if (str[pos] == ',') pos++;
        }
        return Value(obj);
    }

    Value parseArray() {
        Array arr;
        pos++; // skip [
        while (true) {
            skipWhitespace();
            if (pos >= str.size() || str[pos] == ']') {
                pos++;
                break;
            }
            arr.push_back(parse());
            skipWhitespace();
            if (str[pos] == ',') pos++;
        }
        return Value(arr);
    }

    Value parseString() {
        std::string s;
        pos++; // skip "
        while (pos < str.size() && str[pos] != '"') {
            s += str[pos++];
        }
        pos++; // skip closing "
        return Value(s);
    }

    Value parseNumber() {
        size_t start = pos;
        if (str[pos] == '-') pos++;
        while (pos < str.size() && (isdigit(str[pos]) || str[pos] == '.')) {
            pos++;
        }
        std::string numStr = str.substr(start, pos - start);
        return Value(std::stod(numStr));
    }

    Value parseBool() {
        if (str.substr(pos, 4) == "true") {
            pos += 4;
            return Value(true);
        } else if (str.substr(pos, 5) == "false") {
            pos += 5;
            return Value(false);
        }
        return Value(false);
    }
};

inline Value parse(const std::string& s) {
    Parser p(s);
    return p.parse();
}

inline Value parseFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return Value();
    std::stringstream buffer;
    buffer << f.rdbuf();
    return parse(buffer.str());
}

} // namespace json
