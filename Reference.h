#pragma once

#include "Bucket.h"
#include "IteratorHelper.h"

namespace thread_safe {

template <typename KeyT,
          typename ValueTT,
          typename KeyEqualT,
          typename PointerT,
          typename ReferenceT,
          std::size_t BUCKET_COUNT>
struct IteratorHelper;

/*
 * Reference is a class which wraps a pointer to a pointer to
 * some node in Bucket.
 * Dereferencing iterators and operator[] of HashMap gives as 
 * Reference objects.
 * It converts to mapped_type of HashMap implicitly.
 * Provides get(), get_pair(), set(), set_pair() functions to
 * get and set mapped value or the whole pair of node respectively.
 */
template <typename ValueT>
class Reference
{
    template <typename KeyT,
              typename ValueTT,
              typename KeyEqualT,
              typename PointerT,
              typename ReferenceT,
              std::size_t BUCKET_COUNT>
    friend struct IteratorHelper;

public:
    explicit Reference(Node<ValueT>* node)
        : m_node(node)
    {}

    Reference(const Reference& that) = default;
    Reference& operator= (const Reference& that) = default;
    Reference& operator= (const typename ValueT::second_type& value)
    {
        set(value);
        return *this;
    }
    Reference& operator= (const ValueT& value)
    {
        set_pair(value);
        return *this;
    }

    operator typename ValueT::second_type () const
    {
        return get();
    }

    void set(const typename ValueT::second_type& value)
    {
        const auto key = m_node->m_value.load().first;
        m_node->m_value.store(thread_safe::make_pair(key, value));
    }

    void set_pair(const ValueT& value)
    {
        m_node->m_value.store(value);
    }

    typename ValueT::second_type get() const
    {
        return m_node->m_value.load().second;
    }

    ValueT get_pair() const
    {
        return m_node->m_value.load();
    }

private:
    Node<ValueT>* m_node;
};

} // namespace thread_safe
