#pragma once

#include <v2d/support/Assert.hh>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace v2d {

template <typename T, typename SizeType = std::uint32_t>
class Span {
    T *m_data{nullptr};
    SizeType m_size{0};

public:
    constexpr Span() = default;
    constexpr Span(T *data, SizeType size) : m_data(data), m_size(size) {}

    // Allow implicit conversion from `Span<T>` to `Span<void>`.
    constexpr operator Span<void>() { return {data(), size_bytes()}; }
    constexpr operator Span<const void>() const { return {data(), size_bytes()}; }

    constexpr T *begin() const { return m_data; }
    constexpr T *end() const { return m_data + m_size; }

    template <typename U = std::conditional_t<std::is_same_v<T, void>, char, T>>
    constexpr U &operator[](SizeType index) const requires(!std::is_same_v<T, void>) {
        V2D_ASSERT(index < m_size);
        return m_data[index];
    }

    constexpr T *data() const { return m_data; }
    constexpr SizeType size() const { return m_size; }
    constexpr SizeType size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
using LargeSpan = Span<T, std::size_t>;

} // namespace v2d
