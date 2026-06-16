#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace aviwx {

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;

    Json();
    explicit Json(std::nullptr_t);
    explicit Json(bool value);
    explicit Json(double value);
    explicit Json(std::string value);
    explicit Json(Array value);
    explicit Json(Object value);

    static Json parse(const std::string& text);

    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_bool() const;
    double as_number() const;
    const std::string& as_string() const;
    const Array& as_array() const;
    const Object& as_object() const;

    bool contains(const std::string& key) const;
    const Json& at(const std::string& key) const;
    const Json& at(std::size_t index) const;
    std::size_t size() const;

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_;
};

} // namespace aviwx
