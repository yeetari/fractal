#pragma once

#include <v2d/maths/Common.hh>
#include <v2d/support/Array.hh>

#include <cstdint>

namespace v2d {

template <typename T, unsigned ElementCount>
class Vec {
    Array<T, ElementCount> m_elements;

public:
    constexpr Vec() : m_elements{T{}} {}
    template <typename... Ts>
    constexpr Vec(Ts... ts) requires(sizeof...(Ts) == ElementCount) : m_elements{ts...} {}
    constexpr Vec(T t) requires(ElementCount != 1) {
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] = t;
        }
    }

    Vec &operator+=(const Vec &rhs) {
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] += rhs.m_elements[i];
        }
        return *this;
    }
    Vec &operator-=(const Vec &rhs) {
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] -= rhs.m_elements[i];
        }
        return *this;
    }
    Vec &operator*=(const Vec &rhs) {
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] *= rhs.m_elements[i];
        }
        return *this;
    }
    Vec &operator/=(const Vec &rhs) {
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] /= rhs.m_elements[i];
        }
        return *this;
    }

    Vec operator+(const Vec &rhs) const { return Vec(*this) += rhs; }
    Vec operator-(const Vec &rhs) const { return Vec(*this) -= rhs; }
    Vec operator*(const Vec &rhs) const { return Vec(*this) *= rhs; }
    Vec operator/(const Vec &rhs) const { return Vec(*this) /= rhs; }

    Vec abs() const {
        Vec ret;
        for (unsigned i = 0; i < ElementCount; i++) {
            ret.m_elements[i] = v2d::abs(m_elements[i]);
        }
        return ret;
    }

    float dot(const Vec &rhs) const {
        float ret = 0.0f;
        for (unsigned i = 0; i < ElementCount; i++) {
            ret += m_elements[i] * rhs.m_elements[i];
        }
        return ret;
    }

    float magnitude() const { return v2d::sqrt(square_magnitude()); }
    float square_magnitude() const { return dot(*this); }

    Vec &normalise() {
        float inv_mag = 1.0f / magnitude();
        for (unsigned i = 0; i < ElementCount; i++) {
            m_elements[i] *= inv_mag;
        }
        return *this;
    }
    Vec normalised() const { return Vec(*this).normalise(); }

    bool operator<(const Vec &rhs) const {
        for (unsigned i = 0; i < ElementCount; i++) {
            if (m_elements[i] >= rhs.m_elements[i]) {
                return false;
            }
        }
        return true;
    }
    bool operator>(const Vec &rhs) const {
        for (unsigned i = 0; i < ElementCount; i++) {
            if (m_elements[i] <= rhs.m_elements[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr T x() const requires(ElementCount >= 1) { return m_elements[0]; }
    constexpr T y() const requires(ElementCount >= 2) { return m_elements[1]; }
    constexpr T z() const requires(ElementCount >= 3) { return m_elements[2]; }
    constexpr T w() const requires(ElementCount >= 4) { return m_elements[3]; }
};

using Vec2f = Vec<float, 2>;
using Vec2u = Vec<std::uint32_t, 2>;

} // namespace v2d
