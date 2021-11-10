#pragma once

#include <cstddef>

namespace v2d {

template <template <typename, typename...> typename Container, typename... Extra>
class TypeErased {
    Container<std::byte, Extra...> m_container;

public:
    template <typename U>
    Container<U, Extra...> &as() {
        return *reinterpret_cast<Container<U, Extra...> *>(&m_container);
    }
};

} // namespace v2d
