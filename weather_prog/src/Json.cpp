#include "aviwx/Json.h"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace aviwx {
namespace {

class Parser {
public:
    explicit Parser(const std::string& text)
        : text_(text)
    {
    }

    Json parse()
    {
        Json value = parse_value();
        skip_ws();
        if (pos_ != text_.size()) {
            throw error("unexpected trailing characters");
        }
        return value;
    }

private:
    Json parse_value()
    {
        skip_ws();
        if (pos_ >= text_.size()) {
            throw error("unexpected end of input");
        }

        const char c = text_[pos_];
        if (c == '{') {
            return Json(parse_object());
        }
        if (c == '[') {
            return Json(parse_array());
        }
        if (c == '"') {
            return Json(parse_string());
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return Json(parse_number());
        }
        if (match("true")) {
            return Json(true);
        }
        if (match("false")) {
            return Json(false);
        }
        if (match("null")) {
            return Json(nullptr);
        }
        throw error("expected JSON value");
    }

    Json::Object parse_object()
    {
        expect('{');
        Json::Object object;
        skip_ws();
        if (peek('}')) {
            ++pos_;
            return object;
        }

        while (true) {
            skip_ws();
            if (!peek('"')) {
                throw error("expected object key");
            }
            std::string key = parse_string();
            skip_ws();
            expect(':');
            object.emplace(std::move(key), parse_value());
            skip_ws();
            if (peek('}')) {
                ++pos_;
                return object;
            }
            expect(',');
        }
    }

    Json::Array parse_array()
    {
        expect('[');
        Json::Array array;
        skip_ws();
        if (peek(']')) {
            ++pos_;
            return array;
        }

        while (true) {
            array.push_back(parse_value());
            skip_ws();
            if (peek(']')) {
                ++pos_;
                return array;
            }
            expect(',');
        }
    }

    std::string parse_string()
    {
        expect('"');
        std::string out;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return out;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (pos_ >= text_.size()) {
                throw error("unterminated escape sequence");
            }
            const char escaped = text_[pos_++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                out.push_back(escaped);
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                throw error("unsupported escape sequence");
            }
        }
        throw error("unterminated string");
    }

    double parse_number()
    {
        const std::size_t start = pos_;
        if (peek('-')) {
            ++pos_;
        }
        consume_digits();
        if (peek('.')) {
            ++pos_;
            consume_digits();
        }
        if (peek('e') || peek('E')) {
            ++pos_;
            if (peek('+') || peek('-')) {
                ++pos_;
            }
            consume_digits();
        }

        try {
            return std::stod(text_.substr(start, pos_ - start));
        } catch (const std::exception&) {
            throw error("invalid number");
        }
    }

    void consume_digits()
    {
        if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            throw error("expected digit");
        }
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool match(const char* literal)
    {
        const std::string value(literal);
        if (text_.compare(pos_, value.size(), value) == 0) {
            pos_ += value.size();
            return true;
        }
        return false;
    }

    bool peek(char expected) const
    {
        return pos_ < text_.size() && text_[pos_] == expected;
    }

    void expect(char expected)
    {
        skip_ws();
        if (!peek(expected)) {
            throw error(std::string("expected '") + expected + "'");
        }
        ++pos_;
    }

    void skip_ws()
    {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    std::runtime_error error(const std::string& message) const
    {
        return std::runtime_error("JSON parse error at byte " + std::to_string(pos_) + ": " + message);
    }

    const std::string& text_;
    std::size_t pos_{};
};

} // namespace

Json::Json()
    : value_(nullptr)
{
}

Json::Json(std::nullptr_t)
    : value_(nullptr)
{
}

Json::Json(bool value)
    : value_(value)
{
}

Json::Json(double value)
    : value_(value)
{
}

Json::Json(std::string value)
    : value_(std::move(value))
{
}

Json::Json(Array value)
    : value_(std::move(value))
{
}

Json::Json(Object value)
    : value_(std::move(value))
{
}

Json Json::parse(const std::string& text)
{
    return Parser(text).parse();
}

bool Json::is_null() const
{
    return std::holds_alternative<std::nullptr_t>(value_);
}

bool Json::is_bool() const
{
    return std::holds_alternative<bool>(value_);
}

bool Json::is_number() const
{
    return std::holds_alternative<double>(value_);
}

bool Json::is_string() const
{
    return std::holds_alternative<std::string>(value_);
}

bool Json::is_array() const
{
    return std::holds_alternative<Array>(value_);
}

bool Json::is_object() const
{
    return std::holds_alternative<Object>(value_);
}

bool Json::as_bool() const
{
    if (!is_bool()) {
        throw std::runtime_error("JSON value is not a boolean");
    }
    return std::get<bool>(value_);
}

double Json::as_number() const
{
    if (!is_number()) {
        throw std::runtime_error("JSON value is not a number");
    }
    return std::get<double>(value_);
}

const std::string& Json::as_string() const
{
    if (!is_string()) {
        throw std::runtime_error("JSON value is not a string");
    }
    return std::get<std::string>(value_);
}

const Json::Array& Json::as_array() const
{
    if (!is_array()) {
        throw std::runtime_error("JSON value is not an array");
    }
    return std::get<Array>(value_);
}

const Json::Object& Json::as_object() const
{
    if (!is_object()) {
        throw std::runtime_error("JSON value is not an object");
    }
    return std::get<Object>(value_);
}

bool Json::contains(const std::string& key) const
{
    return is_object() && as_object().find(key) != as_object().end();
}

const Json& Json::at(const std::string& key) const
{
    const auto& object = as_object();
    const auto it = object.find(key);
    if (it == object.end()) {
        throw std::runtime_error("missing JSON key: " + key);
    }
    return it->second;
}

const Json& Json::at(std::size_t index) const
{
    const auto& array = as_array();
    if (index >= array.size()) {
        throw std::runtime_error("JSON array index out of range");
    }
    return array[index];
}

std::size_t Json::size() const
{
    if (is_array()) {
        return as_array().size();
    }
    if (is_object()) {
        return as_object().size();
    }
    return 0;
}

} // namespace aviwx
