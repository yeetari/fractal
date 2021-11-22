#pragma once

#include <concepts>
#include <math.h> // NOLINT

namespace v2d {

template <typename T>
constexpr T abs(T val) {
    return val >= T(0) ? val : -val;
}

template <std::signed_integral T>
constexpr T abs(T val) {
    constexpr auto sign_bit_place = sizeof(T) * 8 - 1;
    const T sign_bit = val >> sign_bit_place;
    return (val ^ sign_bit) - sign_bit;
}

template <std::unsigned_integral T>
constexpr T abs(T val) {
    return val;
}

template <typename T>
constexpr T min(T a, T b) {
    return b < a ? b : a;
}

template <typename T>
constexpr T max(T a, T b) {
    return a < b ? b : a;
}

template <typename T>
constexpr T clamp(T val, T min_val, T max_val) {
    return min(max(val, min_val), max_val);
}

constexpr float sqrt(float val) {
    return sqrtf(val);
}

} // namespace v2d
