
#ifndef BEAST_UNIT_TEST_DETAIL_CONST_CONTAINER_HPP
#define BEAST_UNIT_TEST_DETAIL_CONST_CONTAINER_HPP

namespace beast {
namespace unit_test {
namespace detail {


template<class Container>
class const_container
{
private:
    using cont_type = Container;

    cont_type m_cont;

protected:
    cont_type& cont()
    {
        return m_cont;
    }

    cont_type const& cont() const
    {
        return m_cont;
    }

public:
    using value_type = typename cont_type::value_type;
    using size_type = typename cont_type::size_type;
    using difference_type = typename cont_type::difference_type;
    using iterator = typename cont_type::const_iterator;
    using const_iterator = typename cont_type::const_iterator;

    
    bool
    empty() const
    {
        return m_cont.empty();
    }

    
    size_type
    size() const
    {
        return m_cont.size();
    }

    
    
    const_iterator
    begin() const
    {
        return m_cont.cbegin();
    }

    const_iterator
    cbegin() const
    {
        return m_cont.cbegin();
    }

    const_iterator
    end() const
    {
        return m_cont.cend();
    }

    const_iterator
    cend() const
    {
        return m_cont.cend();
    }
    
};

} 
} 
} 

#endif






