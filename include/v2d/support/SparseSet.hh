#pragma once

#include <v2d/support/Assert.hh>
#include <v2d/support/Vector.hh>

#include <utility>

namespace v2d {

template <typename E, typename I>
class SparseSet {
    Vector<I, I> m_dense;
    Vector<I, I> m_sparse;
    Vector<E> m_storage;

public:
    bool contains(I index) const;
    template <typename... Args>
    void insert(I index, Args &&...args);
    void remove(I index);

    auto dense_begin() { return m_dense.begin(); }
    auto dense_end() { return m_dense.end(); };
    auto storage_begin() { return m_storage.begin(); }
    auto storage_end() { return m_storage.end(); };

    E &operator[](I index);
    const E &operator[](I index) const;

    bool empty() const { return m_dense.empty(); }
    I size() const { return m_dense.size(); }
};

template <typename E, typename I>
bool SparseSet<E, I>::contains(I index) const {
    return index < m_sparse.size() && m_sparse[index] < m_dense.size() && m_dense[m_sparse[index]] == index;
}

template <typename E, typename I>
template <typename... Args>
void SparseSet<E, I>::insert(I index, Args &&...args) {
    V2D_ASSERT(index >= m_sparse.size() || !contains(index));
    m_sparse.ensure_size(index + 1);
    m_sparse[index] = m_dense.size();
    m_dense.push(index);
    m_storage.emplace(std::forward<Args>(args)...);
}

template <typename E, typename I>
void SparseSet<E, I>::remove(I index) {
    V2D_ASSERT(contains(index));
    if (m_dense[m_sparse[index]] != m_dense.last()) {
        // TODO: Not well tested.
        m_sparse[m_dense.last()] = m_sparse[index];
        std::swap(m_dense[m_sparse[index]], m_dense.last());
        std::swap(m_storage[m_sparse[index]], m_storage.last());
    }
    m_dense.pop();
    m_storage.pop();
}

template <typename E, typename I>
E &SparseSet<E, I>::operator[](I index) {
    V2D_ASSERT(contains(index));
    return m_storage[m_sparse[index]];
}

template <typename E, typename I>
const E &SparseSet<E, I>::operator[](I index) const {
    V2D_ASSERT(contains(index));
    return m_storage[m_sparse[index]];
}

} // namespace v2d
