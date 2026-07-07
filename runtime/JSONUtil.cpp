// UltraWeb/runtime/JSONUtil.cpp
// Minimal JSON parser/serializer implementation
// Version: 1.0.0
// Last Modified: 2026-07-07
// Author: UltraCanvas Framework

#include "JSONUtil.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace UltraWeb {
namespace Runtime {

namespace {

const JSONValue kNullValue;

struct Parser {
    const char* p;
    const char* end;
    bool ok = true;

    void SkipWs() {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    }

    bool Consume(char c) {
        SkipWs();
        if (p < end && *p == c) { p++; return true; }
        ok = false;
        return false;
    }

    bool Literal(const char* lit) {
        size_t n = std::strlen(lit);
        if (static_cast<size_t>(end - p) >= n && std::memcmp(p, lit, n) == 0) {
            p += n;
            return true;
        }
        ok = false;
        return false;
    }

    JSONValue ParseValue() {
        SkipWs();
        if (p >= end) { ok = false; return JSONValue(); }
        switch (*p) {
            case '{': return ParseObject();
            case '[': return ParseArray();
            case '"': return JSONValue::MakeString(ParseString());
            case 't': Literal("true");  return JSONValue::MakeBool(true);
            case 'f': Literal("false"); return JSONValue::MakeBool(false);
            case 'n': Literal("null");  return JSONValue();
            default:  return ParseNumber();
        }
    }

    std::string ParseString() {
        std::string out;
        if (!Consume('"')) return out;
        while (p < end && *p != '"') {
            char c = *p++;
            if (c == '\\' && p < end) {
                char e = *p++;
                switch (e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        if (end - p < 4) { ok = false; return out; }
                        unsigned code = 0;
                        for (int i = 0; i < 4; i++) {
                            char h = *p++;
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= h - '0';
                            else if (h >= 'a' && h <= 'f') code |= h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') code |= h - 'A' + 10;
                            else { ok = false; return out; }
                        }
                        // UTF-8 encode (BMP only; surrogate pairs pass through
                        // as replacement - sufficient for the marshalling layer)
                        if (code < 0x80) {
                            out += static_cast<char>(code);
                        } else if (code < 0x800) {
                            out += static_cast<char>(0xC0 | (code >> 6));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (code >> 12));
                            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default: ok = false; return out;
                }
            } else {
                out += c;
            }
        }
        Consume('"');
        return out;
    }

    JSONValue ParseNumber() {
        const char* start = p;
        if (p < end && (*p == '-' || *p == '+')) p++;
        while (p < end && (std::isdigit(static_cast<unsigned char>(*p)) ||
                           *p == '.' || *p == 'e' || *p == 'E' ||
                           *p == '-' || *p == '+')) p++;
        std::string num(start, p);
        if (num.empty()) { ok = false; return JSONValue(); }
        try {
            return JSONValue::MakeNumber(std::stod(num));
        } catch (...) {
            ok = false;
            return JSONValue();
        }
    }

    JSONValue ParseArray() {
        JSONValue arr = JSONValue::MakeArray();
        Consume('[');
        SkipWs();
        if (p < end && *p == ']') { p++; return arr; }
        while (ok) {
            arr.Push(ParseValue());
            SkipWs();
            if (p < end && *p == ',') { p++; continue; }
            break;
        }
        Consume(']');
        return arr;
    }

    JSONValue ParseObject() {
        JSONValue obj = JSONValue::MakeObject();
        Consume('{');
        SkipWs();
        if (p < end && *p == '}') { p++; return obj; }
        while (ok) {
            SkipWs();
            std::string key = ParseString();
            Consume(':');
            obj.Set(key, ParseValue());
            SkipWs();
            if (p < end && *p == ',') { p++; continue; }
            break;
        }
        Consume('}');
        return obj;
    }
};

} // namespace

const JSONValue& JSONValue::At(size_t i) const {
    return i < arrayValue.size() ? arrayValue[i] : kNullValue;
}

const JSONValue& JSONValue::Get(const std::string& key) const {
    auto it = objectValue.find(key);
    return it != objectValue.end() ? it->second : kNullValue;
}

std::string JSONValue::Quote(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
    return out;
}

std::string JSONValue::Serialize() const {
    switch (type) {
        case Type::Null:    return "null";
        case Type::Boolean: return boolValue ? "true" : "false";
        case Type::Number: {
            if (std::isfinite(numberValue) &&
                numberValue == static_cast<int64_t>(numberValue) &&
                std::fabs(numberValue) < 1e15) {
                return std::to_string(static_cast<int64_t>(numberValue));
            }
            std::ostringstream oss;
            oss << numberValue;
            return oss.str();
        }
        case Type::String:  return Quote(stringValue);
        case Type::Array: {
            std::string out = "[";
            for (size_t i = 0; i < arrayValue.size(); i++) {
                if (i) out += ',';
                out += arrayValue[i].Serialize();
            }
            return out + "]";
        }
        case Type::Object: {
            std::string out = "{";
            bool first = true;
            for (const auto& kv : objectValue) {
                if (!first) out += ',';
                first = false;
                out += Quote(kv.first) + ":" + kv.second.Serialize();
            }
            return out + "}";
        }
    }
    return "null";
}

JSONValue JSONValue::Parse(const std::string& text, bool& ok) {
    Parser parser;
    parser.p = text.data();
    parser.end = text.data() + text.size();
    JSONValue v = parser.ParseValue();
    parser.SkipWs();
    ok = parser.ok && parser.p == parser.end;
    return ok ? v : JSONValue();
}

} // namespace Runtime
} // namespace UltraWeb
