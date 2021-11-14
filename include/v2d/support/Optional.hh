#pragma once

#include <v2d/support/Array.hh>
#include <v2d/support/Assert.hh>

#include <cstddef>
#include <utility>

namespace v2d {

template <typename T>
class Optional {
    alignas(T) Array<std::byte, sizeof(T)> m_data{};
    bool m_present{false};

public:
    constexpr Optional() = default;
    constexpr Optional(const T &value) : m_present(true) { new (m_data.data()) T(value); }
    constexpr Optional(T &&value) : m_present(true) { new (m_data.data()) T(std::move(value)); }
    constexpr Optional(const Optional &) = delete;
    constexpr Optional(Optional &&other) noexcept : m_present(other.m_present) {
        if (other) {
            new (m_data.data()) T(std::move(*other));
            other.clear();
        }
    }
    constexpr ~Optional() { clear(); }

    Optional &operator=(const Optional &) = delete;
    Optional &operator=(Optional &&) = delete;

    constexpr void clear();
    template <typename... Args>
    constexpr T &emplace(Args &&...args);

    constexpr explicit operator bool() const { return m_present; }
    constexpr T &operator*();
    constexpr T *operator->();
    constexpr const T &operator*() const;
    constexpr const T *operator->() const;
};

template <typename T>
constexpr void Optional<T>::clear() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        if (m_present) {
            operator*().~T();
        }
    }
    m_present = false;
}

template <typename T>
template <typename... Args>
constexpr T &Optional<T>::emplace(Args &&...args) {
    clear();
    new (m_data.data()) T(std::forward<Args>(args)...);
    m_present = true;
    return operator*();
}


template <typename T>
constexpr T &Optional<T>::operator*() {
    V2D_ASSERT(m_present);
    return *reinterpret_cast<T *>(m_data.data());
}

template <typename T>
constexpr T *Optional<T>::operator->() {
    V2D_ASSERT(m_present);
    return reinterpret_cast<T *>(m_data.data());
}

template <typename T>
constexpr const T &Optional<T>::operator*() const {
    V2D_ASSERT(m_present);
    return *reinterpret_cast<const T *>(m_data.data());
}

template <typename T>
constexpr const T *Optional<T>::operator->() const {
    V2D_ASSERT(m_present);
    return reinterpret_cast<const T *>(m_data.data());
}

} // namespace v2d
