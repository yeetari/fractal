#pragma once

#include <v2d/support/Assert.hh>
#include <v2d/support/Span.hh>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace v2d {

template <typename T, typename SizeType = std::uint32_t>
class Vector {
    T *m_data{nullptr};
    SizeType m_capacity{0};
    SizeType m_size{0};

public:
    constexpr Vector() = default;
    template <typename... Args>
    explicit Vector(SizeType size, Args &&...args);
    Vector(const Vector &);
    Vector(Vector &&other) noexcept
        : m_data(std::exchange(other.m_data, nullptr)), m_capacity(std::exchange(other.m_capacity, 0u)),
          m_size(std::exchange(other.m_size, 0u)) {}
    ~Vector();

    Vector &operator=(const Vector &) = delete;
    Vector &operator=(Vector &&) noexcept;

    void clear();
    void ensure_capacity(SizeType capacity);
    template <typename... Args>
    void grow(SizeType size, Args &&...args);
    void reallocate(SizeType capacity);

    template <typename... Args>
    T &emplace(Args &&...args);
    void push(const T &elem);
    void push(T &&elem);
    void pop();

    Span<T> span() { return {m_data, m_size}; }
    Span<const T> span() const { return {m_data, m_size}; }

    T *begin() { return m_data; }
    T *end() { return m_data + m_size; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + m_size; }

    T &operator[](SizeType index);
    const T &operator[](SizeType index) const;

    T &first() { return begin()[0]; }
    const T &first() const { return begin()[0]; }
    T &last() { return begin()[m_size - 1]; }
    const T &last() const { return begin()[m_size - 1]; }

    bool empty() const { return m_size == 0; }
    T *data() const { return m_data; }
    SizeType capacity() const { return m_capacity; }
    SizeType size() const { return m_size; }
    SizeType size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
using LargeVector = Vector<T, std::size_t>;

template <typename T, typename SizeType>
template <typename... Args>
Vector<T, SizeType>::Vector(SizeType size, Args &&...args) {
    grow(size, std::forward<Args>(args)...);
}

template <typename T, typename SizeType>
Vector<T, SizeType>::Vector(const Vector &other) {
    ensure_capacity(other.size());
    m_size = other.size();
    if constexpr (!std::is_trivially_copyable_v<T>) {
        for (auto *data = m_data; const auto &elem : other) {
            new (data++) T(elem);
        }
    } else {
        std::memcpy(m_data, other.data(), other.size_bytes());
    }
}

template <typename T, typename SizeType>
Vector<T, SizeType>::~Vector() {
    clear();
    delete[] reinterpret_cast<std::uint8_t *>(m_data);
}

template <typename T, typename SizeType>
Vector<T, SizeType> &Vector<T, SizeType>::operator=(Vector &&other) noexcept {
    if (this != &other) {
        clear();
        m_data = std::exchange(other.m_data, nullptr);
        m_capacity = std::exchange(other.m_capacity, 0u);
        m_size = std::exchange(other.m_size, 0u);
    }
    return *this;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::clear() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    }
    m_size = 0;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::ensure_capacity(SizeType capacity) {
    if (capacity > m_capacity) {
        reallocate(std::max(m_capacity * 2 + 1, capacity));
    }
}

template <typename T, typename SizeType>
template <typename... Args>
void Vector<T, SizeType>::grow(SizeType size, Args &&...args) {
    if (size <= m_size) {
        return;
    }
    ensure_capacity(size);
    if constexpr (!std::is_trivially_copyable_v<T> || sizeof...(Args) != 0) {
        for (SizeType i = m_size; i < size; i++) {
            new (begin() + i) T(std::forward<Args>(args)...);
        }
    } else {
        std::memset(begin() + m_size, 0, size * sizeof(T) - m_size * sizeof(T));
    }
    m_size = size;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::reallocate(SizeType capacity) {
    V2D_ASSERT(capacity >= m_size);
    T *new_data = reinterpret_cast<T *>(new std::uint8_t[capacity * sizeof(T)]);
    if constexpr (!std::is_trivially_copyable_v<T>) {
        for (auto *data = new_data; auto &elem : *this) {
            new (data++) T(std::move(elem));
        }
        for (auto *elem = end(); elem != begin();) {
            (--elem)->~T();
        }
    } else {
        if (m_size != 0) {
            std::memcpy(new_data, m_data, size_bytes());
        }
    }
    delete[] reinterpret_cast<std::uint8_t *>(m_data);
    m_data = new_data;
    m_capacity = capacity;
}

template <typename T, typename SizeType>
template <typename... Args>
T &Vector<T, SizeType>::emplace(Args &&...args) {
    ensure_capacity(m_size + 1);
    new (end()) T(std::forward<Args>(args)...);
    return (*this)[m_size++];
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::push(const T &elem) {
    ensure_capacity(m_size + 1);
    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(end(), &elem, sizeof(T));
    } else {
        new (end()) T(elem);
    }
    m_size++;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::push(T &&elem) {
    ensure_capacity(m_size + 1);
    new (end()) T(std::move(elem));
    m_size++;
}

template <typename T, typename SizeType>
void Vector<T, SizeType>::pop() {
    V2D_ASSERT(!empty());
    m_size--;
    end()->~T();
}

template <typename T, typename SizeType>
T &Vector<T, SizeType>::operator[](SizeType index) {
    V2D_ASSERT(index < m_size);
    return begin()[index];
}

template <typename T, typename SizeType>
const T &Vector<T, SizeType>::operator[](SizeType index) const {
    V2D_ASSERT(index < m_size);
    return begin()[index];
}

} // namespace v2d
