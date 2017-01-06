#pragma once

#include <cstddef>
#include <atomic>
#include <mutex>

namespace thread_safe {

/*
 * A structure which stores two instances of any type.
 * It's like the std::pair and even implicitly constructs from it.
 * The important difference, which also is the reason of creation
 * of this class is that the std::pair is not trivially copyable,
 * which is the required from std::atomic
 */
template <typename T1, typename T2>
struct Pair
{
    typedef T1 first_type;
    typedef T2 second_type;

    first_type first;
    second_type second;

    Pair()
        : first(T1())
        , second(T2())
    {};

    Pair(const T1& f, const T2& s)
        : first(f)
        , second(s)
    {}

    template <typename U1, typename U2>
    Pair(U1&& f, U2&& s)
        : first(std::forward<U1>(f))
        , second(std::forward<U2>(s))
    {}

    template <typename U1, typename U2>
    Pair(const Pair<U1, U2>& that)
        : first(that.first)
        , second(that.second)
    {}

    template <typename U1, typename U2>
    Pair(const std::pair<U1, U2>& that)
        : first(that.first)
        , second(that.second)
    {}

    template <typename U1, typename U2>
    Pair(Pair<U1, U2>&& that)
        : first(std::move(that.first))
        , second(std::move(that.second))
    {}
};

template <typename T1, typename T2>
Pair<T1, T2> make_pair(const T1& f, const T2& s)
{
    return Pair<T1, T2>(f, s);
}

/*
 * Each Bucket object is a doubly linked list, and Node
 * is the type of nodes in that list
 */
template <typename ValueT>
struct Node
{
    Node()
        : m_prev(nullptr)
        , m_next(nullptr)
        , m_value(ValueT())
    {}
    Node(const Node&) = delete;
    Node& operator= (const Node&) = delete;
    ~Node()
    {}

    Node* m_prev;
    Node* m_next;
    std::atomic<ValueT> m_value;
};

/*
 * HashMap contains an array of Buckets as storage.
 * Each Bucket is a doubly linked list.
 */
template <typename KeyT,
          typename ValueT,
          typename KeyEqualT>
class Bucket
{
public:
    Bucket();
    Bucket(const Bucket&);
    Bucket(Bucket&&);
    Bucket& operator= (const Bucket&);
    Bucket& operator= (Bucket&&);
    ~Bucket();

    Pair<Node<ValueT>*, bool> insert(const ValueT& value);
    Node<ValueT>* insert_or_assign(const ValueT& value);
    Node<ValueT>* find(const KeyT& key);
    const Node<ValueT>* find(const KeyT& key) const;
    void erase(const KeyT& key);
    void erase(Node<ValueT>* node);
    void clear();
    std::size_t size() const;
    bool empty() const;
    Node<ValueT>* begin();
    const Node<ValueT>* begin() const;
    Node<ValueT>* end();
    const Node<ValueT>* end() const;

private:
    mutable std::recursive_mutex m_mutex;
    std::size_t m_size;
    Node<ValueT>* m_end;
    KeyEqualT m_key_equal;
};

#define TEMPLATE_DECL template <typename KeyT, typename ValueT, typename KeyEqualT>
#define CLASS_NAME Bucket<KeyT, ValueT, KeyEqualT>

/*
 * Default constructor
 */
TEMPLATE_DECL
CLASS_NAME::Bucket()
    : m_size(0)
    , m_end(nullptr)
{
    m_end = new Node<ValueT>();
    m_end->m_next = m_end;
    m_end->m_prev = m_end;
}

/*
 * Copy constructor
 */
TEMPLATE_DECL
CLASS_NAME::Bucket(const Bucket& that)
    : m_size(that.m_size)
    , m_end(nullptr)
{
    m_end = new Node<ValueT>();
    m_end->m_next = m_end;
    m_end->m_prev = m_end;
    
    // Locks the source so that it won't be modified during the copy
    std::lock_guard<std::recursive_mutex> lck(that.m_mutex);
    Node<ValueT>* node = that.begin();
    while (node != that.end()) {
        Node<ValueT>* new_node = new Node<ValueT>();
        new_node->m_value.store(node->m_value.load());
        new_node->m_next = m_end;
        new_node->m_prev = m_end->m_prev;
        new_node->m_next->m_prev = new_node;
        new_node->m_prev->m_next = new_node;
        node = node->m_next;
    }
}

/*
 * Move constructor
 */
TEMPLATE_DECL
CLASS_NAME::Bucket(Bucket&& that)
    : m_size(that.m_size)
    , m_end(nullptr)
{
    std::lock_guard<std::recursive_mutex> lck(that.m_mutex);
    m_end = that.m_end;
    that.m_end = nullptr;
    m_size = that.m_size;
}

/*
 * Copy assignment
 */
TEMPLATE_DECL
CLASS_NAME& CLASS_NAME::operator= (const Bucket& that)
{
    if (&that != this) {
        // std::lock is to avoid deadlock in case of cross assignment,
        // i.e. "a = b" in one thread and "b = a" in the other
        std::unique_lock<std::recursive_mutex> lck_this(m_mutex, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lck_that(that.m_mutex, std::defer_lock);
        std::lock(lck_this, lck_that);
        clear();
        const Node<ValueT>* node = that.begin();
        while (node != that.end()) {
            Node<ValueT>* new_node = new Node<ValueT>();
            new_node->m_value.store(node->m_value.load());
            new_node->m_next = m_end;
            new_node->m_prev = m_end->m_prev;
            new_node->m_next->m_prev = new_node;
            new_node->m_prev->m_next = new_node;
            node = node->m_next;
        }
        m_size = that.m_size;
    }
    return *this;
}

/*
 * Move assignment
 */
TEMPLATE_DECL
CLASS_NAME& CLASS_NAME::operator= (Bucket&& that)
{
    if (&that != this) {
        std::unique_lock<std::recursive_mutex> lck_this(m_mutex, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lck_that(that.m_mutex, std::defer_lock);
        std::lock(lck_this, lck_that);
        clear();
        m_end = that.m_end;
        that.m_end = nullptr;
        m_size = that.m_size;
    }
    return *this;
}

/*
 * Destructor
 */
TEMPLATE_DECL
CLASS_NAME::~Bucket()
{
    clear();
}

/*
 * Insert
 * Locks the container, inserts the given pair if there is no pair
 * with the same key, unlocks the container
 */
TEMPLATE_DECL
Pair<Node<ValueT>*, bool> CLASS_NAME::insert(const ValueT& value)
{
    Node<ValueT>* result = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        result = find(value.first);
        if (result != m_end) {
            return thread_safe::make_pair(result, false);
        }
        result = new Node<ValueT>();
        result->m_next = m_end;
        result->m_prev = m_end->m_prev;
        result->m_next->m_prev = result;
        result->m_prev->m_next = result;
        ++m_size;
    }
    result->m_value.store(value);
    return thread_safe::make_pair(result, true);
}

/*
 * Insert or assign
 * Locks the container, inserts the given pair if there is no pair
 * with the same key or replaces the existing with the new otherwise,
 * unlocks the container
 */
TEMPLATE_DECL
Node<ValueT>* CLASS_NAME::insert_or_assign(const ValueT& value)
{
    Node<ValueT>* result = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lck(m_mutex);
        result = find(value.first);
        if (result == m_end) {
            result = new Node<ValueT>();
            result->m_next = m_end;
            result->m_prev = m_end->m_prev;
            result->m_next->m_prev = result;
            result->m_prev->m_next = result;
            ++m_size;
        }
    }
    result->m_value.store(value);
    return result;
}

/*
 * Find
 * Returns a pointer to the node with the key provided
 */
TEMPLATE_DECL
Node<ValueT>* CLASS_NAME::find(const KeyT& key)
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    Node<ValueT>* node = begin();
    while (node != end()) {
        if (m_key_equal(node->m_value.load().first, key)) {
            return node;
        }
        node = node->m_next;
    }
    return node;
}

/*
 * Find
 * Returns a pointer to the node with the key provided for const objects
 */
TEMPLATE_DECL
const Node<ValueT>* CLASS_NAME::find(const KeyT& key) const
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    const Node<ValueT>* node = begin();
    while (node != end()) {
        if (m_key_equal(node->m_value.load().first, key)) {
            return node;
        }
        node = node->m_next;
    }
    return node;
}

/*
 * Erase
 * Erases the node with the key if such one exists
 */
TEMPLATE_DECL
void CLASS_NAME::erase(const KeyT& key)
{
    erase(find(key));
}

/*
 * Erase
 * Erases the node
 */
TEMPLATE_DECL
void CLASS_NAME::erase(Node<ValueT>* node)
{
    if (node == m_end) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    node->m_next->m_prev = node->m_prev;
    node->m_prev->m_next = node->m_next;
    delete node;
    node = nullptr;
    --m_size;
}

/*
 * Erases all nodes
 */
TEMPLATE_DECL
void CLASS_NAME::clear()
{
    Node<ValueT>* node = nullptr;
    while ((node = begin()) != end()) {
        erase(node);
    }
}

/*
 * Returns the number of elements
 */
TEMPLATE_DECL
std::size_t CLASS_NAME::size() const
{
    return m_size;
}

/*
 * Returns true if the bucket is empty and false otherwise
 */
TEMPLATE_DECL
bool CLASS_NAME::empty() const
{
    std::lock_guard<std::recursive_mutex> lck(m_mutex);
    return begin() == end();
}

/*
 * Returns a pointer to the first node
 */
TEMPLATE_DECL
Node<ValueT>* CLASS_NAME::begin()
{
    return m_end->m_next;
}

TEMPLATE_DECL
const Node<ValueT>* CLASS_NAME::begin() const
{
    return m_end->m_next;
}

/*
 * Returns a pointer to the dummy nil node
 */
TEMPLATE_DECL
Node<ValueT>* CLASS_NAME::end()
{
    return m_end;
}

TEMPLATE_DECL
const Node<ValueT>* CLASS_NAME::end() const
{
    return m_end;
}

#undef TEMPLATE_DECL
#undef CLASS_NAME

} // namespace thread_safe
