

#ifndef RIPPLE_BASICS_BUFFER_H_INCLUDED
#define RIPPLE_BASICS_BUFFER_H_INCLUDED

#include <ripple/basics/Slice.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

namespace ripple {


class Buffer
{
private:
    std::unique_ptr<std::uint8_t[]> p_;
    std::size_t size_ = 0;

public:
    using const_iterator = std::uint8_t const*;

    Buffer() = default;

    
    explicit
    Buffer (std::size_t size)
        : p_ (size ? new std::uint8_t[size] : nullptr)
        , size_ (size)
    {
    }

    
    Buffer (void const* data, std::size_t size)
        : Buffer (size)
    {
        if (size)
            std::memcpy(p_.get(), data, size);
    }

    
    Buffer (Buffer const& other)
        : Buffer (other.p_.get(), other.size_)
    {
    }

    
    Buffer& operator= (Buffer const& other)
    {
        if (this != &other)
        {
            if (auto p = alloc (other.size_))
                std::memcpy (p, other.p_.get(), size_);
        }
        return *this;
    }

    
    Buffer (Buffer&& other) noexcept
        : p_ (std::move(other.p_))
        , size_ (other.size_)
    {
        other.size_ = 0;
    }

    
    Buffer& operator= (Buffer&& other) noexcept
    {
        if (this != &other)
        {
            p_ = std::move(other.p_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    
    explicit
    Buffer (Slice s)
        : Buffer (s.data(), s.size())
    {
    }

    
    Buffer& operator= (Slice s)
    {
        assert (s.size() == 0 || size_ == 0 ||
            s.data() < p_.get() ||
            s.data() >= p_.get() + size_);

        if (auto p = alloc (s.size()))
            std::memcpy (p, s.data(), s.size());
        return *this;
    }

    
    std::size_t
    size() const noexcept
    {
        return size_;
    }

    bool
    empty () const noexcept
    {
        return 0 == size_;
    }

    operator Slice() const noexcept
    {
        if (! size_)
            return Slice{};
        return Slice{ p_.get(), size_ };
    }

    
    
    std::uint8_t const*
    data() const noexcept
    {
        return p_.get();
    }

    std::uint8_t*
    data() noexcept
    {
        return p_.get();
    }
    

    
    void
    clear() noexcept
    {
        p_.reset();
        size_ = 0;
    }

    
    std::uint8_t*
    alloc (std::size_t n)
    {
        if (n != size_)
        {
            p_.reset(n ? new std::uint8_t[n] : nullptr);
            size_ = n;
        }
        return p_.get();
    }

    void*
    operator()(std::size_t n)
    {
        return alloc(n);
    }

    const_iterator
    begin() const noexcept
    {
        return p_.get();
    }

    const_iterator
    cbegin() const noexcept
    {
        return p_.get();
    }

    const_iterator
    end() const noexcept
    {
        return p_.get() + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return p_.get() + size_;
    }
};

inline bool operator==(Buffer const& lhs, Buffer const& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    if (lhs.size() == 0)
        return true;

    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline bool operator!=(Buffer const& lhs, Buffer const& rhs) noexcept
{
    return !(lhs == rhs);
}

} 

#endif








