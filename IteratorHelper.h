#pragma once

#include "Reference.h"

namespace thread_safe { 

/*
 * Forward Iterator for HashMap
 */
template <typename KeyT,
          typename ValueT,
          typename KeyEqualT,
          typename PointerT,
          typename ReferenceT,
          std::size_t BUCKET_COUNT>
struct IteratorHelper
{
public:
    typedef std::ptrdiff_t difference_type;
    typedef ValueT value_type;
    typedef PointerT pointer;
    typedef ReferenceT reference;
    typedef std::forward_iterator_tag iterator_category;

public:
    explicit IteratorHelper(Bucket<KeyT, ValueT, KeyEqualT>* buckets,
                            const std::size_t index,
                            Node<ValueT>* node)
        : m_buckets(buckets)
        , m_index(index)
        , m_ref(node)
    {
        if (m_buckets[m_index].end() == node) {
            m_index = BUCKET_COUNT;
        }
    }
    IteratorHelper(const IteratorHelper& that) = default;
    IteratorHelper& operator= (const IteratorHelper& that) = default;
    ~IteratorHelper() = default;

    template <typename FwdIter>
    IteratorHelper(FwdIter that)
        : m_buckets(that.m_buckets)
        , m_index(that.m_index)
        , m_ref(that.m_ref)
    {}

    reference operator* ()
    {
        return m_ref;
    }

    const reference operator* () const
    {
        return m_ref;
    }

    pointer operator-> ()
    {
        return &m_ref;
    }

    const pointer operator-> () const
    {
        return &m_ref;
    }

    IteratorHelper& operator++ ()
    {
        m_ref = ReferenceT(m_ref.m_node->m_next);
        if (m_ref.m_node == m_buckets[m_index].end()) {
            do {
                ++m_index;
            } while (m_index != BUCKET_COUNT && m_buckets[m_index].empty());
            if (m_index != BUCKET_COUNT) {
                m_ref = ReferenceT(m_buckets[m_index].begin());
            }
        }
        return *this;
    }

    IteratorHelper operator++ (int)
    {
        IteratorHelper tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator== (const IteratorHelper& that)
    {
        if (m_index == BUCKET_COUNT && that.m_index == BUCKET_COUNT) {
            return true;
        }
        if (m_ref.m_node == that.m_ref.m_node) {
            return true;
        }
        return false;
    }

    bool operator!= (const IteratorHelper& that)
    {
        return !(*this == that);
    }

private:
    Bucket<KeyT, ValueT, KeyEqualT>* m_buckets;
    std::size_t m_index;
    ReferenceT m_ref;
};

} // namespace thread_safe
