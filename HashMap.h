#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>

#include "IteratorHelper.h"

namespace thread_safe {
    
/*
* An associative thread safe container which provides interface for
* insertion, deletion and lookup via iterators and not only.
* This container is pretty much like std::unordered_map STL container.
* The advantage of HashMap is its thread safety.
* The disadvantage is that the number of buckets is fixed and predefined,
* which is not much of a disadvantage if user knows the approximate number
* of elements to be stored.
*/
template <typename KeyT,
          typename MappedT,
          std::size_t BUCKET_COUNT,
          typename HashT = std::hash<KeyT>,
          typename KeyEqualT = std::equal_to<KeyT> >
class HashMap
{
public:
    typedef KeyT key_type;
    typedef MappedT mapped_type;
    typedef Pair<const KeyT, MappedT> value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef HashT hasher;
    typedef KeyEqualT key_equal;
    typedef Reference<value_type> reference;
    typedef const reference const_reference;
    typedef reference* pointer;
    typedef const_reference* const_pointer;

public:
    typedef IteratorHelper<key_type,
                           value_type,
                           key_equal,
                           pointer,
                           reference,
                           BUCKET_COUNT> iterator;
    typedef IteratorHelper<key_type,
                           value_type,
                           key_equal,
                           const_pointer,
                           const_reference,
                           BUCKET_COUNT> const_iterator;

    /* Constructors */
public:
    explicit HashMap(const hasher& hash = hasher());
    template <typename InputIt>
    HashMap(InputIt first,
            InputIt last,
            const hasher& hash = hasher());
    HashMap(const std::initializer_list<value_type>& il,
            const hasher& hash = hasher());
    HashMap(const HashMap& that);
    HashMap(HashMap&& that);
    HashMap& operator= (const HashMap& that);
    HashMap& operator= (HashMap&& that);
    HashMap& operator= (const std::initializer_list<value_type>& il);
    ~HashMap();

    /* Mutators */
public:
    Pair<iterator, bool> insert(const key_type& key, const mapped_type& value);
    Pair<iterator, bool> insert(const value_type& value);
    iterator insert_or_assign(const key_type& key, const mapped_type& value);
    iterator insert_or_assign(const value_type& value);
    void erase(const key_type& key);
    iterator erase(iterator position);
    iterator find(const key_type& key);
    reference operator[] (const key_type& key);
    void clear();

    /* Selectors */
public:
    const_iterator find(const key_type& key) const;
    size_type size() const;
    bool empty() const;

    /* Iterators */
public:
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    
    /* Hasher */
public:
    hasher& get_hasher();
    const hasher& get_hasher() const;

    /* Private members and helper functions */
private:
    Bucket<key_type, value_type, KeyEqualT>* m_buckets;
    hasher m_hasher;
    mutable std::recursive_mutex m_mutex;
};


/* Implementation of member functions */
#define TEMPLATE_DECL template <typename KeyT,\
                                typename MappedT,\
                                std::size_t BUCKET_COUNT,\
                                typename HashT,\
                                typename KeyEqualT>
#define CLASS_NAME HashMap<KeyT, MappedT, BUCKET_COUNT, HashT, KeyEqualT>

/*
 * Default constructor with empty buckets
 */
TEMPLATE_DECL
CLASS_NAME::HashMap(const hasher& hash)
    : m_buckets(new Bucket<key_type, value_type, KeyEqualT>[BUCKET_COUNT])
    , m_hasher(hash)
{}

/*
 * Iterator based constructor
 */
TEMPLATE_DECL
template <typename InputIt>
CLASS_NAME::HashMap(InputIt first,
                    InputIt last,
                    const hasher& hash)
    : m_buckets(new Bucket<key_type, value_type, KeyEqualT>[BUCKET_COUNT])
    , m_hasher(hash)
{
    for (auto it = first; it != last; ++it) {
        insert(thread_safe::make_pair(it->first, it->second));
    }
}

/*
 * Copy constructor
 */
TEMPLATE_DECL
CLASS_NAME::HashMap(const HashMap& that)
    : m_buckets(new Bucket<key_type, value_type, KeyEqualT>[BUCKET_COUNT])
    , m_hasher(that.m_hasher)
{
    // This lock is to ensure that the source container won't be
    // affected during the copy
    std::lock_guard<std::recursive_mutex> lck(that.m_mutex);
    std::copy(that.m_buckets, that.m_buckets + BUCKET_COUNT, m_buckets);
}

/*
 * Move constructor
 */
TEMPLATE_DECL
CLASS_NAME::HashMap(HashMap&& that)
    : m_buckets(nullptr)
    , m_hasher(that.m_hasher)
{
    // This lock is to ensure that the source container won't be
    // affected during the move
    std::lock_guard<std::recursive_mutex> lck(that.m_mutex);
    m_buckets = that.m_buckets;
    that.m_buckets = nullptr;
}

/*
 * Constructor for braced initialization
 */
TEMPLATE_DECL
CLASS_NAME::HashMap(const std::initializer_list<value_type>& il,
                    const hasher& hash)
    : m_buckets(new Bucket<key_type, value_type, KeyEqualT>[BUCKET_COUNT])
    , m_hasher(hash)
{
    for (const auto& value : il) {
        insert(thread_safe::make_pair(value.first, value.second));
    }
}

/*
 * Copy assignment
 */
TEMPLATE_DECL
CLASS_NAME& CLASS_NAME::operator= (const HashMap& that) 
{
    if (&that != this) {
        // std::lock is to avoid deadlock in case of cross assignment,
        // i.e. "a = b" in one thread and "b = a" in the other
        std::unique_lock<std::recursive_mutex> lck_this(m_mutex, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lck_that(that.m_mutex, std::defer_lock);
        std::lock(lck_this, lck_that);

        clear();
        m_hasher = that.m_hasher;
        std::copy(that.m_buckets, that.m_buckets + BUCKET_COUNT, m_buckets);
    }
    return *this;
}

/*
 * Move assignment
 */
TEMPLATE_DECL
CLASS_NAME& CLASS_NAME::operator= (HashMap&& that)
{
    if (&that != this) {
        // std::lock is to avoid deadlock in case of cross assignment,
        // i.e. "a = b" in one thread and "b = a" in the other
        std::unique_lock<std::recursive_mutex> lck_this(m_mutex, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lck_that(that.m_mutex, std::defer_lock);
        std::lock(lck_this, lck_that);

        clear();
        m_hasher = that.m_hasher;
        m_buckets = that.m_buckets;
        that.m_buckets = nullptr;
    }
    return *this;
}

/*
 * Assignment for initializer_list
 */
TEMPLATE_DECL
CLASS_NAME& CLASS_NAME::operator= (const std::initializer_list<value_type>& il)
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    for (const auto& value : il) {
        insert(value);
    }
}

/*
 * Destructor
 */
TEMPLATE_DECL
CLASS_NAME::~HashMap()
{
    clear();
    delete[] m_buckets;
    m_buckets = nullptr;
}

/*
 * Insertion
 * This functions are not locked with mutex, because insert function of each
 * bucket is thread safe, and this allows to insert in different buckets
 * concurrently
 */
TEMPLATE_DECL
Pair<typename CLASS_NAME::iterator, bool> CLASS_NAME::insert(const key_type& key, const mapped_type& value)
{
    return insert(thread_safe::make_pair(key, value));
}

TEMPLATE_DECL
Pair<typename CLASS_NAME::iterator, bool> CLASS_NAME::insert(const value_type& value)
{
    const auto bucket_index = m_hasher(value.first) % BUCKET_COUNT;
    auto result = m_buckets[bucket_index].insert(value);
    return thread_safe::make_pair(iterator(m_buckets, bucket_index, result.first), result.second);
}

TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::insert_or_assign(const key_type& key, const mapped_type& value)
{
    return insert_or_assign(thread_safe::make_pair(key, value));
}

TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::insert_or_assign(const value_type& value)
{
    const auto bucket_index = m_hasher(value.first) % BUCKET_COUNT;
    auto result = m_buckets[bucket_index].insert_or_assign(value);
    return iterator(m_buckets, bucket_index, result);
}

/*
 * Deletion
 * This functions are not locked with mutex, because erase function of each
 * bucket is thread safe, and this allows to erase from different buckets
 * concurrently
 */
TEMPLATE_DECL
void CLASS_NAME::erase(const key_type& key)
{
    const auto bucket_index = m_hasher(key) % BUCKET_COUNT;
    m_buckets[bucket_index].erase(key);
}

TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::erase(iterator position)
{
    iterator next = position;
    ++next;
    const auto key = position->get_pair().first;
    erase(key);
    return next;
}

/*
 * Find for non const objects
 */
TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::find(const key_type& key)
{
    const auto bucket_index = m_hasher(key) % BUCKET_COUNT;
    auto result = m_buckets[bucket_index].find(key);
    return iterator(m_buckets, bucket_index, result);
}

/*
 * operator[]
 * Returns a reference object to an existing pair if one with the provided
 * key exists, or to a new pair if not
 * To get the pair from reference object it provides get_pair() function
 * To get the mapped value from reference object it provides get() function
 * To modify itself it provides set_pair() and set() functions respectively
 */
TEMPLATE_DECL
typename CLASS_NAME::reference CLASS_NAME::operator[] (const key_type& key)
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    auto iter = find(key);
    if (iter == end()) {
        iter = insert(key, mapped_type()).first;
    }
    return *iter;
}

/*
 * Clears all buckets
 */
TEMPLATE_DECL
void CLASS_NAME::clear()
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    if (m_buckets != nullptr) {
        for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
            m_buckets[i].clear();
        }
    }
}

/*
 * Find for const objects
 */
TEMPLATE_DECL
typename CLASS_NAME::const_iterator CLASS_NAME::find(const key_type& key) const
{
    const auto bucket_index = m_hasher(key) % BUCKET_COUNT;
    auto result = m_buckets[bucket_index].find(key);
    return const_iterator(m_buckets, bucket_index, result);
}

/*
 * Returns the number of objects in container
 */
TEMPLATE_DECL
typename CLASS_NAME::size_type CLASS_NAME::size() const
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    size_type s = 0;
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        s += m_buckets[i].size();
    }
    return s;
}

/*
 * Returns true if the container is empty and false otherwise
 */
TEMPLATE_DECL
bool CLASS_NAME::empty() const
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    for (std::size_t i = 0; i < BUCKET_COUNT; ++i) {
        if (!m_buckets[i].empty()) {
            return false;
        }
    }
    return true;
}

/*
 * Returns a forward iterator to the begin of the container
 */
TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::begin()
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    std::size_t bucket_index = 0;
    while (bucket_index < BUCKET_COUNT && m_buckets[bucket_index].empty()) {
        ++bucket_index;
    }
    if (bucket_index == BUCKET_COUNT) {
        return iterator(m_buckets, BUCKET_COUNT, nullptr);
    }
    return iterator(m_buckets, bucket_index, m_buckets[bucket_index].begin());
}

/*
 * Returns a forward const iterator to the begin of the container
 */
TEMPLATE_DECL
typename CLASS_NAME::const_iterator CLASS_NAME::begin() const
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    std::size_t bucket_index = 0;
    while (bucket_index < BUCKET_COUNT && m_buckets[bucket_index].empty()) {
        ++bucket_index;
    }
    if (bucket_index == BUCKET_COUNT) {
        return const_iterator(m_buckets, BUCKET_COUNT, nullptr);
    }
    return const_iterator(m_buckets, bucket_index, m_buckets[bucket_index].begin());
}

/*
 * Returns a forward iterator to the end of the container
 */
TEMPLATE_DECL
typename CLASS_NAME::iterator CLASS_NAME::end()
{
    return iterator(m_buckets, BUCKET_COUNT, nullptr);
}

/*
 * Returns a forward const iterator to the end of the container
 */
TEMPLATE_DECL
typename CLASS_NAME::const_iterator CLASS_NAME::end() const
{
    return const_iterator(m_buckets, BUCKET_COUNT, nullptr);
}

#undef TEMPLATE_DECL
#undef CLASS_NAME

} // namespace thread_safe
