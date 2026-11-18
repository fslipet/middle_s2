#include <cstdint>
#include <gtest/gtest.h>
#include <sstream>

#include "scan.hpp"

template <typename Tuple, std::size_t... i>
std::string tuple_to_string_impl(const Tuple &t, std::index_sequence<i...>) {
    std::ostringstream ostr;
    ((ostr << std::get<i>(t) << ' '), ...);
    return ostr.str();
}

template <typename... Args>
std::string tuple_to_string(const std::tuple<Args...> &t) {
    return tuple_to_string_impl(t, std::index_sequence_for<Args...>{});
}

TEST(ScanTest, Empty) {
    ASSERT_EQ(stdx::scan("", "").error().message, "template parameters are empty");
    ASSERT_EQ(stdx::scan("a", "a").error().message, "template parameters are empty");
    ASSERT_EQ(stdx::scan<int8_t>("", "").error().message, "input string is empty");
    ASSERT_EQ(stdx::scan<int8_t>("asd", "").error().message, "format string is empty");
}

TEST(ScanTest, NotMatchesNumbers) {
    ASSERT_EQ(stdx::scan<uint8_t>("11", "{}{}").error().message,
              "number of placeholders and number of template parameters not matches");
    ASSERT_EQ((stdx::scan<uint8_t, uint16_t>("1", "{}").error().message),
              "number of placeholders and number of template parameters not matches");
}

TEST(ScanTest, Integer) {
    ASSERT_EQ(std::get<0>(stdx::scan<int8_t>("1", "{%d}").value().GetResult()), 1);
    ASSERT_EQ(std::get<0>(stdx::scan<int16_t>("-1", "{%d}").value().GetResult()), -1);
    ASSERT_EQ(std::get<0>(stdx::scan<int32_t>("0", "{%d}").value().GetResult()), 0);
    ASSERT_EQ(std::get<0>(stdx::scan<int64_t>("1152921504606846975", "{%d}").value().GetResult()), 1152921504606846975);

    ASSERT_EQ(std::get<0>(stdx::scan<int8_t>("1", "{}").value().GetResult()), 1);
    ASSERT_EQ(std::get<0>(stdx::scan<int16_t>("-1", "{}").value().GetResult()), -1);
    ASSERT_EQ(std::get<0>(stdx::scan<int32_t>("0", "{}").value().GetResult()), 0);
    ASSERT_EQ(std::get<0>(stdx::scan<int64_t>("1152921504606846975", "{}").value().GetResult()), 1152921504606846975);
}

TEST(ScanTest, NaturalInteger) {
    ASSERT_EQ(std::get<0>(stdx::scan<uint8_t>("10", "{%u}").value().GetResult()), 10);
    ASSERT_EQ(std::get<0>(stdx::scan<uint16_t>("65535", "{%u}").value().GetResult()), 65535);
    ASSERT_EQ(std::get<0>(stdx::scan<uint32_t>("0", "{%u}").value().GetResult()), 0);
    ASSERT_EQ(std::get<0>(stdx::scan<uint64_t>("18446744073709551615", "{%u}").value().GetResult()),
              18446744073709551615u);
    ASSERT_EQ(stdx::scan<uint16_t>("-1", "{%u}").error().message, "error of converting");

    ASSERT_EQ(std::get<0>(stdx::scan<uint8_t>("1", "{}").value().GetResult()), 1);
    ASSERT_EQ(std::get<0>(stdx::scan<uint16_t>("65535", "{}").value().GetResult()), 65535);
    ASSERT_EQ(std::get<0>(stdx::scan<uint32_t>("0", "{}").value().GetResult()), 0);
    ASSERT_EQ(std::get<0>(stdx::scan<uint64_t>("18446744073709551615", "{}").value().GetResult()),
              18446744073709551615u);
    ASSERT_EQ(stdx::scan<uint16_t>("-1", "{%u}").error().message, "error of converting");
}

TEST(ScanTest, Float) {
    ASSERT_FLOAT_EQ(std::get<0>(stdx::scan<float>("1.1", "{%f}").value().GetResult()), 1.1);
    ASSERT_FLOAT_EQ(std::get<0>(stdx::scan<float>("1", "{%f}").value().GetResult()), 1.);
    ASSERT_DOUBLE_EQ(std::get<0>(stdx::scan<double>("112233.1122334455", "{%f}").value().GetResult()),
                     112233.1122334455);
    ASSERT_DOUBLE_EQ(std::get<0>(stdx::scan<double>("-112233.1122334455", "{%f}").value().GetResult()),
                     -112233.1122334455);

    ASSERT_FLOAT_EQ(std::get<0>(stdx::scan<float>("1.1", "{}").value().GetResult()), 1.1);
    ASSERT_FLOAT_EQ(std::get<0>(stdx::scan<float>("1", "{}").value().GetResult()), 1.);
    ASSERT_DOUBLE_EQ(std::get<0>(stdx::scan<double>("112233.1122334455", "{}").value().GetResult()), 112233.1122334455);
    ASSERT_DOUBLE_EQ(std::get<0>(stdx::scan<double>("-112233.1122334455", "{}").value().GetResult()),
                     -112233.1122334455);
}

TEST(ScanTest, String) {
    ASSERT_EQ(std::get<0>(stdx::scan<std::string>("aaasssddd", "{%s}").value().GetResult()), "aaasssddd");
    ASSERT_EQ(std::get<0>(stdx::scan<std::string_view>("aaasssddd", "{%s}").value().GetResult()), "aaasssddd");
    ASSERT_EQ(std::get<0>(stdx::scan<std::string>("aaasssddd", "{}").value().GetResult()), "aaasssddd");
    ASSERT_EQ(std::get<0>(stdx::scan<std::string_view>("aaasssddd", "{}").value().GetResult()), "aaasssddd");
}

TEST(ScanTest, ConstTypes) {
    auto result = stdx::scan<const int16_t, const std::string, std::string>("10 vbvb aw 3.1", "{%d} {%s} aw {}");
    ASSERT_TRUE(result);
    
    ASSERT_EQ(tuple_to_string(result.value().GetResult()), "10 vbvb 3.1 ");
}

TEST(ScanTest, VolatileTypes) {
    auto result = stdx::scan<volatile int16_t, std::string, volatile double>("10 vbvb aw 3.1", "{%d} {%s} aw {}");
    ASSERT_TRUE(result);
    
    ASSERT_EQ(tuple_to_string(result.value().GetResult()), "10 vbvb 3.1 ");
}

TEST(ScanTest, ConstVolatileTypes) {
    auto result = stdx::scan<const volatile int16_t, const std::string, const volatile double>("10 vbvb aw 3.1", "{%d} {%s} aw {}");
    ASSERT_TRUE(result);
    
    ASSERT_EQ(tuple_to_string(result.value().GetResult()), "10 vbvb 3.1 ");
}

TEST(ScanTest, ArbitraryStrings) {
    {
        auto result = tuple_to_string(stdx::scan<int16_t, std::string, double, std::string>(
                                       "vbvb 10, qweee 10.11 !234 vbvb", "vbvb {%d}, {%s} {%f} {%s} vbvb")
                                       .value()
                                       .GetResult());
        ASSERT_EQ(result, "10 qweee 10.11 !234 ");
    }
    {
        auto result = tuple_to_string(
            stdx::scan<uint32_t, std::string, float, int32_t>("15, zsdv qweee vbvb 10.11 zsdv zsdv zsdv !234 3",
                                                              "{%u}, zsdv qweee {%s} {%f} zsdv zsdv zsdv !234 {%d}")
                .value()
                .GetResult());
        ASSERT_EQ(result, "15 vbvb 10.11 3 ");
    }
    {
        auto result =
            tuple_to_string(stdx::scan<uint32_t, uint16_t, int16_t, std::string, double, std::string>(
                             "65536     12     -19 zsdv 3.15    ARR2", "{%u}     {%u}     {%d} {%s} {%f}    {%s}")
                             .value()
                             .GetResult());
        ASSERT_EQ(result, "65536 12 -19 zsdv 3.15 ARR2 ");
    }
    {
        auto result =
            tuple_to_string(stdx::scan<uint32_t, uint16_t, int16_t, std::string, std::string, std::string>(
                             "65536     12     -19 zsdv 3.15    ARR2", "{}     {}     {} {} {}    {}")
                             .value()
                             .GetResult());
        ASSERT_EQ(result, "65536 12 -19 zsdv 3.15 ARR2 ");
    }
}

TEST(ScanTest, UnsupportedTypes) {
    ASSERT_EQ(stdx::scan<char>("1", "{}").error().message, "unsupported type");
    ASSERT_EQ(stdx::scan<std::vector<int>>("1", "{}").error().message, "unsupported type");
}

TEST(ScanTest, RefOrPointers) {
    ASSERT_EQ(stdx::scan<int32_t*>("1", "{}").error().message, "template parameters are reference or pointers");
    ASSERT_EQ(stdx::scan<std::string&>("1", "{}").error().message, "template parameters are reference or pointers");
}

TEST(ScanTest, ConvertError) {
    {
        auto result = stdx::scan<int16_t, std::string>("vbvb 10 aw", "{%d} {%s} aw");
        ASSERT_FALSE(result);
        ASSERT_EQ(result.error().message, "error of converting");
    }
    {
        auto result = stdx::scan<double, std::string>("vbvb 10 aw", "{%f} {%s} aw");
        ASSERT_FALSE(result);
        ASSERT_EQ(result.error().message, "error of converting");
    }
}

TEST(ScanTest, MatchingError) {
    auto result = stdx::scan<int16_t, std::string, std::string>("10 vbvb aw 3.1", "{%u} {%s} aw {}");

    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, "matching error. expected type: %d, parsed type: %u");
}

TEST(ScanTest, ParsingError) {
    auto result = stdx::scan<int16_t, std::string, std::string>("10 vbvb aw", "{%d} {%s} aw {}");

    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, "Unformatted text in input and format string are different");
}

