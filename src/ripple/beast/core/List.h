

#ifndef BEAST_INTRUSIVE_LIST_H_INCLUDED
#define BEAST_INTRUSIVE_LIST_H_INCLUDED

#include <ripple/beast/core/Config.h>

#include <iterator>
#include <type_traits>

namespace beast {

template <typename, typename>
class List;

namespace detail {



template <typename T, typename U>
struct CopyConst
{
    explicit CopyConst() = default;

    using type = typename std::remove_const <U>::type;
};

template <typename T, typename U>
struct CopyConst <T const, U>
{
    explicit CopyConst() = default;

    using type = typename std::remove_const <U>::type const;
};


template <typename T, typename Tag>
class ListNode
{
private:
    using value_type = T;

    friend class List<T, Tag>;

    template <typename>
    friend class ListIterator;

    ListNode* m_next;
    ListNode* m_prev;
};


template <typename N>
class ListIterator : public std::iterator <
    std::bidirectional_iterator_tag, std::size_t>
{
public:
    using value_type = typename beast::detail::CopyConst <
        N, typename N::value_type>::type;
    using pointer = value_type*;
    using reference = value_type&;
    using size_type = std::size_t;

    ListIterator (N* node = nullptr) noexcept
        : m_node (node)
    {
    }

    template <typename M>
    ListIterator (ListIterator <M> const& other) noexcept
        : m_node (other.m_node)
    {
    }

    template <typename M>
    bool operator== (ListIterator <M> const& other) const noexcept
    {
        return m_node == other.m_node;
    }

    template <typename M>
    bool operator!= (ListIterator <M> const& other) const noexcept
    {
        return ! ((*this) == other);
    }

    reference operator* () const noexcept
    {
        return dereference ();
    }

    pointer operator-> () const noexcept
    {
        return &dereference ();
    }

    ListIterator& operator++ () noexcept
    {
        increment ();
        return *this;
    }

    ListIterator operator++ (int) noexcept
    {
        ListIterator result (*this);
        increment ();
        return result;
    }

    ListIterator& operator-- () noexcept
    {
        decrement ();
        return *this;
    }

    ListIterator operator-- (int) noexcept
    {
        ListIterator result (*this);
        decrement ();
        return result;
    }

private:
    reference dereference () const noexcept
    {
        return static_cast <reference> (*m_node);
    }

    void increment () noexcept
    {
        m_node = m_node->m_next;
    }

    void decrement () noexcept
    {
        m_node = m_node->m_prev;
    }

    N* m_node;
};

}


template <typename T, typename Tag = void>
class List
{
public:
    using Node = typename detail::ListNode <T, Tag>;

    using value_type      = T;
    using pointer         = value_type*;
    using reference       = value_type&;
    using const_pointer   = value_type const*;
    using const_reference = value_type const&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator       = detail::ListIterator <Node>;
    using const_iterator = detail::ListIterator <Node const>;

    
    List ()
    {
        m_head.m_prev = nullptr; 
        m_tail.m_next = nullptr; 
        clear ();
    }

    List(List const&) = delete;
    List& operator= (List const&) = delete;

    
    bool empty () const noexcept
    {
        return size () == 0;
    }

    
    size_type size () const noexcept
    {
        return m_size;
    }

    
    reference front () noexcept
    {
        return element_from (m_head.m_next);
    }

    
    const_reference front () const noexcept
    {
        return element_from (m_head.m_next);
    }

    
    reference back () noexcept
    {
        return element_from (m_tail.m_prev);
    }

    
    const_reference back () const noexcept
    {
        return element_from (m_tail.m_prev);
    }

    
    iterator begin () noexcept
    {
        return iterator (m_head.m_next);
    }

    
    const_iterator begin () const noexcept
    {
        return const_iterator (m_head.m_next);
    }

    
    const_iterator cbegin () const noexcept
    {
        return const_iterator (m_head.m_next);
    }

    
    iterator end () noexcept
    {
        return iterator (&m_tail);
    }

    
    const_iterator end () const noexcept
    {
        return const_iterator (&m_tail);
    }

    
    const_iterator cend () const noexcept
    {
        return const_iterator (&m_tail);
    }

    
    void clear () noexcept
    {
        m_head.m_next = &m_tail;
        m_tail.m_prev = &m_head;
        m_size = 0;
    }

    
    iterator insert (iterator pos, T& element) noexcept
    {
        Node* node = static_cast <Node*> (&element);
        node->m_next = &*pos;
        node->m_prev = node->m_next->m_prev;
        node->m_next->m_prev = node;
        node->m_prev->m_next = node;
        ++m_size;
        return iterator (node);
    }

    
    void insert (iterator pos, List& other) noexcept
    {
        if (!other.empty ())
        {
            Node* before = &*pos;
            other.m_head.m_next->m_prev = before->m_prev;
            before->m_prev->m_next = other.m_head.m_next;
            other.m_tail.m_prev->m_next = before;
            before->m_prev = other.m_tail.m_prev;
            m_size += other.m_size;
            other.clear ();
        }
    }

    
    iterator erase (iterator pos) noexcept
    {
        Node* node = &*pos;
        ++pos;
        node->m_next->m_prev = node->m_prev;
        node->m_prev->m_next = node->m_next;
        --m_size;
        return pos;
    }

    
    iterator push_front (T& element) noexcept
    {
        return insert (begin (), element);
    }

    
    T& pop_front () noexcept
    {
        T& element (front ());
        erase (begin ());
        return element;
    }

    
    iterator push_back (T& element) noexcept
    {
        return insert (end (), element);
    }

    
    T& pop_back () noexcept
    {
        T& element (back ());
        erase (--end ());
        return element;
    }

    
    void swap (List& other) noexcept
    {
        List temp;
        temp.append (other);
        other.append (*this);
        append (temp);
    }

    
    iterator prepend (List& list) noexcept
    {
        return insert (begin (), list);
    }

    
    iterator append (List& list) noexcept
    {
        return insert (end (), list);
    }

    
    iterator iterator_to (T& element) const noexcept
    {
        return iterator (static_cast <Node*> (&element));
    }

    
    const_iterator const_iterator_to (T const& element) const noexcept
    {
        return const_iterator (static_cast <Node const*> (&element));
    }

private:
    reference element_from (Node* node) noexcept
    {
        return *(static_cast <pointer> (node));
    }

    const_reference element_from (Node const* node) const noexcept
    {
        return *(static_cast <const_pointer> (node));
    }

private:
    size_type m_size;
    Node m_head;
    Node m_tail;
};

}

#endif








