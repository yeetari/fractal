#pragma once

#include <array>
#include <cstdint>

template <typename T, unsigned ElementCount>
class Vec {
    std::array<T, ElementCount> m_elements;

public:
    constexpr Vec() : m_elements{T{}} {}
    template <typename... Ts>
    constexpr Vec(Ts... ts) requires(sizeof...(Ts) == ElementCount) : m_elements{ts...} {}

    constexpr T x() const requires(ElementCount >= 1) { return m_elements[0]; }
    constexpr T y() const requires(ElementCount >= 2) { return m_elements[1]; }
    constexpr T z() const requires(ElementCount >= 3) { return m_elements[2]; }
    constexpr T w() const requires(ElementCount >= 4) { return m_elements[3]; }
};

using Vec2f = Vec<float, 2>;
using Vec2u = Vec<std::uint32_t, 2>;
