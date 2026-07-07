// UltraWeb/runtime/JSONUtil.h
// Minimal JSON parser/serializer for the C++/JS marshalling layer
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework
//
// Deliberately small: just what UCApi needs to decode __uc_native argument
// arrays and encode results. Not a general-purpose JSON library.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace UltraWeb {
namespace Runtime {

class JSONValue {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    JSONValue() : type(Type::Null), boolValue(false), numberValue(0) {}

    static JSONValue MakeNull()                   { return JSONValue(); }
    static JSONValue MakeBool(bool v)             { JSONValue j; j.type = Type::Boolean; j.boolValue = v; return j; }
    static JSONValue MakeNumber(double v)         { JSONValue j; j.type = Type::Number; j.numberValue = v; return j; }
    static JSONValue MakeString(const std::string& v) { JSONValue j; j.type = Type::String; j.stringValue = v; return j; }
    static JSONValue MakeArray()                  { JSONValue j; j.type = Type::Array; return j; }
    static JSONValue MakeObject()                 { JSONValue j; j.type = Type::Object; return j; }

    Type GetType() const   { return type; }
    bool IsNull() const    { return type == Type::Null; }
    bool IsBool() const    { return type == Type::Boolean; }
    bool IsNumber() const  { return type == Type::Number; }
    bool IsString() const  { return type == Type::String; }
    bool IsArray() const   { return type == Type::Array; }
    bool IsObject() const  { return type == Type::Object; }

    bool GetBool(bool fallback = false) const     { return IsBool() ? boolValue : fallback; }
    double GetNumber(double fallback = 0) const   { return IsNumber() ? numberValue : fallback; }
    int GetInt(int fallback = 0) const            { return IsNumber() ? static_cast<int>(numberValue) : fallback; }
    const std::string& GetString() const          { return stringValue; }

    // Array access
    size_t Size() const { return arrayValue.size(); }
    const JSONValue& At(size_t i) const;
    void Push(const JSONValue& v) { arrayValue.push_back(v); }

    // Object access
    bool Has(const std::string& key) const { return objectValue.count(key) != 0; }
    const JSONValue& Get(const std::string& key) const;
    void Set(const std::string& key, const JSONValue& v) { objectValue[key] = v; }
    const std::map<std::string, JSONValue>& Members() const { return objectValue; }

    // Serialization
    std::string Serialize() const;

    // Parsing; returns Null value with ok=false on malformed input
    static JSONValue Parse(const std::string& text, bool& ok);

    // Escapes a string for embedding in JSON output (adds quotes)
    static std::string Quote(const std::string& s);

private:
    Type type;
    bool boolValue;
    double numberValue;
    std::string stringValue;
    std::vector<JSONValue> arrayValue;
    std::map<std::string, JSONValue> objectValue;
};

} // namespace Runtime
} // namespace UltraWeb
