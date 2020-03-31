

#ifndef RIPPLE_BASICS_SLICE_H_INCLUDED
#define RIPPLE_BASICS_SLICE_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/basics/strHex.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <type_traits>

namespace ripple {


class Slice
{
private:
    std::uint8_t const* data_ = nullptr;
    std::size_t size_ = 0;

public:
    using const_iterator = std::uint8_t const*;

    
    Slice() noexcept = default;

    Slice (Slice const&) noexcept = default;
    Slice& operator= (Slice const&) noexcept = default;

    
    Slice (void const* data, std::size_t size) noexcept
        : data_ (reinterpret_cast<std::uint8_t const*>(data))
        , size_ (size)
    {
    }

    
    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    
    std::size_t
    size() const noexcept
    {
        return size_;
    }

    
    std::uint8_t const*
    data() const noexcept
    {
        return data_;
    }

    
    std::uint8_t
    operator[](std::size_t i) const noexcept
    {
        assert(i < size_);
        return data_[i];
    }

    
    
    Slice&
    operator+= (std::size_t n)
    {
        if (n > size_)
            Throw<std::domain_error> ("too small");
        data_ += n;
        size_ -= n;
        return *this;
    }

    Slice
    operator+ (std::size_t n) const
    {
        Slice temp = *this;
        return temp += n;
    }
    


    const_iterator
    begin() const noexcept
    {
        return data_;
    }

    const_iterator
    cbegin() const noexcept
    {
        return data_;
    }

    const_iterator
    end() const noexcept
    {
        return data_ + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return data_ + size_;
    }
};


template <class Hasher>
inline
void
hash_append (Hasher& h, Slice const& v)
{
    h(v.data(), v.size());
}

inline
bool
operator== (Slice const& lhs, Slice const& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    if (lhs.size() == 0)
        return true;

    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline
bool
operator!= (Slice const& lhs, Slice const& rhs) noexcept
{
    return !(lhs == rhs);
}

inline
bool
operator< (Slice const& lhs, Slice const& rhs) noexcept
{
    return std::lexicographical_compare(
        lhs.data(), lhs.data() + lhs.size(),
            rhs.data(), rhs.data() + rhs.size());
}


template <class Stream>
Stream& operator<<(Stream& s, Slice const& v)
{
    s << strHex(v);
    return s;
}

template <class T, std::size_t N>
std::enable_if_t<
    std::is_same<T, char>::value ||
        std::is_same<T, unsigned char>::value,
    Slice
>
makeSlice (std::array<T, N> const& a)
{
    return Slice(a.data(), a.size());
}

template <class T, class Alloc>
std::enable_if_t<
    std::is_same<T, char>::value ||
        std::is_same<T, unsigned char>::value,
    Slice
>
makeSlice (std::vector<T, Alloc> const& v)
{
    return Slice(v.data(), v.size());
}

template <class Traits, class Alloc>
Slice
makeSlice (std::basic_string<char, Traits, Alloc> const& s)
{
    return Slice(s.data(), s.size());
}

} 

#endif








