#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

// здесь ваш код
template <typename T, typename... Ts>
concept IsAnyOf = (std::same_as<std::remove_cv_t<T>, Ts> || ...);

template <typename T>
concept IsInteger = IsAnyOf<T, int8_t, int16_t, int32_t, int64_t>;

template <typename T>
concept IsNaturalInteger = IsAnyOf<T, uint8_t, uint16_t, uint32_t, uint64_t>;

template <typename T>
concept IsFloat = IsAnyOf<T, float, double>;

template <typename T>
concept IsString = IsAnyOf<T, std::string, std::string_view>;

template <typename T>
concept IsNumeric = IsInteger<T> || IsNaturalInteger<T> || IsFloat<T>;

template <typename T>
concept IsAllowed = IsNumeric<T> || IsString<T>;
//================

constexpr scan_error MatchingError(std::string_view fmt, std::string_view specifier) {
    return scan_error(std::format("matching error. expected type: {}, parsed type: {}", specifier, fmt));
};

inline std::pair<bool, std::string_view> CheckMatchingTypeImp(std::string_view type_specifier, std::string_view fmt) {
    return {fmt == type_specifier || fmt == "", type_specifier};
}

template <IsInteger T>
std::pair<bool, std::string_view> CheckMatchingType(std::string_view fmt) {
    return CheckMatchingTypeImp("%d", fmt);
}

template <IsNaturalInteger T>
std::pair<bool, std::string_view> CheckMatchingType(std::string_view fmt) {
    return CheckMatchingTypeImp("%u", fmt);
}

template <IsFloat T>
std::pair<bool, std::string_view> CheckMatchingType(std::string_view fmt) {
    return CheckMatchingTypeImp("%f", fmt);
}

template <IsString T>
std::pair<bool, std::string_view> CheckMatchingType(std::string_view fmt) {
    return CheckMatchingTypeImp("%s", fmt);
}

//=========parse_value=======
template <IsString T>
std::expected<std::remove_cv_t<T>, scan_error> parse_value(std::string_view value) {
    return std::remove_cv_t<T>(value.begin(), value.end());
}

template <IsNumeric T>
std::expected<std::remove_cv_t<T>, scan_error> parse_value(std::string_view value) {
    if (std::remove_cv_t<T> result; std::from_chars(value.begin(), value.end(), result).ec == std::errc{}) {
        return result;
    }
    return std::unexpected(scan_error("error of converting"));
}

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    if constexpr (IsAllowed<T>) {
        if (auto [res, specifier] = CheckMatchingType<T>(fmt); !res) {
            return std::unexpected(MatchingError(fmt, specifier));
        }
        return parse_value<T>(input);
    }
    return std::unexpected(scan_error("unsupported type"));
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details