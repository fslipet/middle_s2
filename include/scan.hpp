#pragma once

#include "parse.hpp"
#include "types.hpp"
#include <expected>

namespace stdx {

template <typename It, typename T, typename... Ts>
std::expected<std::tuple<T, Ts...>, details::scan_error> Parse(It it_v, It it_f) {
    auto parse_result = details::parse_value_with_format<T>(*it_v, *it_f);
    if (!parse_result) {
        return std::unexpected(parse_result.error());
    }
    if constexpr (sizeof...(Ts) == 0) {
        return std::make_tuple(parse_result.value());
    } else {
        auto imp_result = Parse<It, Ts...>(++it_v, ++it_f);
        if (!imp_result) {
            return std::unexpected(imp_result.error());
        }
        return std::tuple_cat(std::make_tuple(parse_result.value()), imp_result.value());
    }
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    auto is_ref_or_point = [] -> bool { return ((std::is_reference_v<Ts> || std::is_pointer_v<Ts>) || ...); };

    if constexpr (sizeof...(Ts) == 0) {
        return std::unexpected(details::scan_error{"template parameters are empty"});
    } else if constexpr (is_ref_or_point()) {
        return std::unexpected(details::scan_error{"template parameters are reference or pointers"});
    } else {
        if (input.empty()) {
            return std::unexpected(details::scan_error{"input string is empty"});
        }
        if (format.empty()) {
            return std::unexpected(details::scan_error{"format string is empty"});
        }

        auto parsed_result = details::parse_sources<Ts...>(input, format);
        if (!parsed_result) {
            return std::unexpected(parsed_result.error());
        }

        auto [vec_of_formats, vec_of_values] = parsed_result.value();

        if (vec_of_formats.size() != sizeof...(Ts)) {
            return std::unexpected(
                details::scan_error{"number of placeholders and number of template parameters not matches"});
        }

        auto result = Parse<decltype(vec_of_formats.begin()), Ts...>(vec_of_values.begin(), vec_of_formats.begin());
        if (!result) {
            return std::unexpected(std::move(result).error());
        }

        return details::scan_result(std::move(result).value());
    }
}

}  // namespace stdx
