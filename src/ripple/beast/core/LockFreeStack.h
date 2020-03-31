

#ifndef BEAST_INTRUSIVE_LOCKFREESTACK_H_INCLUDED
#define BEAST_INTRUSIVE_LOCKFREESTACK_H_INCLUDED

#include <atomic>
#include <iterator>
#include <type_traits>

namespace beast {


template <class Container, bool IsConst>
class LockFreeStackIterator
    : public std::iterator <
        std::forward_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        typename std::conditional <IsConst,
            typename Container::const_pointer,
            typename Container::pointer>::type,
        typename std::conditional <IsConst,
            typename Container::const_reference,
            typename Container::reference>::type>
{
protected:
    using Node = typename Container::Node;
    using NodePtr = typename std::conditional <
        IsConst, Node const*, Node*>::type;

public:
    using value_type = typename Container::value_type;
    using pointer = typename std::conditional <IsConst,
        typename Container::const_pointer,
        typename Container::pointer>::type;
    using reference = typename std::conditional <IsConst,
        typename Container::const_reference,
        typename Container::reference>::type;

    LockFreeStackIterator ()
        : m_node ()
    {
    }

    LockFreeStackIterator (NodePtr node)
        : m_node (node)
    {
    }

    template <bool OtherIsConst>
    explicit LockFreeStackIterator (LockFreeStackIterator <Container, OtherIsConst> const& other)
        : m_node (other.m_node)
    {
    }

    LockFreeStackIterator& operator= (NodePtr node)
    {
        m_node = node;
        return static_cast <LockFreeStackIterator&> (*this);
    }

    LockFreeStackIterator& operator++ ()
    {
        m_node = m_node->m_next.load ();
        return static_cast <LockFreeStackIterator&> (*this);
    }

    LockFreeStackIterator operator++ (int)
    {
        LockFreeStackIterator result (*this);
        m_node = m_node->m_next;
        return result;
    }

    NodePtr node() const
    {
        return m_node;
    }

    reference operator* () const
    {
        return *this->operator-> ();
    }

    pointer operator-> () const
    {
        return static_cast <pointer> (m_node);
    }

private:
    NodePtr m_node;
};


template <class Container, bool LhsIsConst, bool RhsIsConst>
bool operator== (LockFreeStackIterator <Container, LhsIsConst> const& lhs,
                 LockFreeStackIterator <Container, RhsIsConst> const& rhs)
{
    return lhs.node() == rhs.node();
}

template <class Container, bool LhsIsConst, bool RhsIsConst>
bool operator!= (LockFreeStackIterator <Container, LhsIsConst> const& lhs,
                 LockFreeStackIterator <Container, RhsIsConst> const& rhs)
{
    return lhs.node() != rhs.node();
}



template <class Element, class Tag = void>
class LockFreeStack
{
public:
    class Node
    {
    public:
        Node ()
            : m_next (nullptr)
        { }

        explicit Node (Node* next)
            : m_next (next)
        { }

        Node(Node const&) = delete;
        Node& operator= (Node const&) = delete;

    private:
        friend class LockFreeStack;

        template <class Container, bool IsConst>
        friend class LockFreeStackIterator;

        std::atomic <Node*> m_next;
    };

public:
    using value_type      = Element;
    using pointer         = Element*;
    using reference       = Element&;
    using const_pointer   = Element const*;
    using const_reference = Element const&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator        = LockFreeStackIterator <
        LockFreeStack <Element, Tag>, false>;
    using const_iterator  = LockFreeStackIterator <
        LockFreeStack <Element, Tag>, true>;

    LockFreeStack ()
        : m_end (nullptr)
        , m_head (&m_end)
    {
    }

    LockFreeStack(LockFreeStack const&) = delete;
    LockFreeStack& operator= (LockFreeStack const&) = delete;

    
    bool empty() const
    {
        return m_head.load () == &m_end;
    }

    
    bool push_front (Node* node)
    {
        bool first;
        Node* old_head = m_head.load (std::memory_order_relaxed);
        do
        {
            first = (old_head == &m_end);
            node->m_next = old_head;
        }
        while (!m_head.compare_exchange_strong (old_head, node,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        return first;
    }

    
    Element* pop_front ()
    {
        Node* node = m_head.load ();
        Node* new_head;
        do
        {
            if (node == &m_end)
                return nullptr;
            new_head = node->m_next.load ();
        }
        while (!m_head.compare_exchange_strong (node, new_head,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        return static_cast <Element*> (node);
    }

    
    
    iterator begin ()
    {
        return iterator (m_head.load ());
    }

    iterator end ()
    {
        return iterator (&m_end);
    }

    const_iterator begin () const
    {
        return const_iterator (m_head.load ());
    }

    const_iterator end () const
    {
        return const_iterator (&m_end);
    }

    const_iterator cbegin () const
    {
        return const_iterator (m_head.load ());
    }

    const_iterator cend () const
    {
        return const_iterator (&m_end);
    }
    

private:
    Node m_end;
    std::atomic <Node*> m_head;
};

}

#endif








