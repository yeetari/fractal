#pragma once

#include <v2d/support/Assert.hh>
#include <v2d/support/Vector.hh>

#include <utility>

namespace v2d {

template <typename E, typename I>
class SparseSet {
    Vector<std::pair<I, E>, I> m_dense;
    Vector<I, I> m_sparse;

public:
    bool contains(I index) const;
    template <typename... Args>
    void insert(I index, Args &&...args);
    void remove(I index);

    auto begin() { return m_dense.begin(); }
    auto end() { return m_dense.end(); };

    E &operator[](I index);
    const E &operator[](I index) const;

    bool empty() const { return m_dense.empty(); }
    I size() const { return m_dense.size(); }
};

template <typename E, typename I>
bool SparseSet<E, I>::contains(I index) const {
    return m_sparse[index] < m_dense.size() && m_dense[m_sparse[index]].first == index;
}

template <typename E, typename I>
template <typename... Args>
void SparseSet<E, I>::insert(I index, Args &&...args) {
    V2D_ASSERT(index >= m_sparse.size() || !contains(index));
    m_sparse.ensure_size(index + 1);
    m_sparse[index] = m_dense.size();
    m_dense.emplace(std::piecewise_construct, std::forward_as_tuple(index),
                    std::forward_as_tuple(std::forward<Args>(args)...));
}

template <typename E, typename I>
void SparseSet<E, I>::remove(I index) {
    V2D_ASSERT(contains(index));
    m_sparse[m_dense.last().first] = m_sparse[index];
    std::swap(m_dense[m_sparse[index]], m_dense.last());
    m_dense.pop();
}

template <typename E, typename I>
E &SparseSet<E, I>::operator[](I index) {
    V2D_ASSERT(contains(index));
    return m_dense[m_sparse[index]].second;
}

template <typename E, typename I>
const E &SparseSet<E, I>::operator[](I index) const {
    V2D_ASSERT(contains(index));
    return m_dense[m_sparse[index]].second;
}

} // namespace v2d
