

#ifndef BEAST_CRYPTO_SECURE_ERASE_H_INCLUDED
#define BEAST_CRYPTO_SECURE_ERASE_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <new>

namespace beast {

namespace detail {

class secure_erase_impl
{
private:
    struct base
    {
        virtual void operator()(
            void* dest, std::size_t bytes) const = 0;
        virtual ~base() = default;
        base() = default;
        base(base const&) = delete;
        base& operator=(base const&) = delete;
    };

    struct impl : base
    {
        void operator()(
            void* dest, std::size_t bytes) const override
        {
            char volatile* volatile p =
                const_cast<volatile char*>(
                    reinterpret_cast<char*>(dest));
            if (bytes == 0)
                return;
            do
            {
                *p = 0;
            }
            while(*p++ == 0 && --bytes);
        }
    };

    char buf_[sizeof(impl)];
    base& erase_;

public:
    secure_erase_impl()
        : erase_(*new(buf_) impl)
    {
    }

    void operator()(
        void* dest, std::size_t bytes) const
    {
        return erase_(dest, bytes);
    }
};

}


template <class = void>
void
secure_erase (void* dest, std::size_t bytes)
{
    static detail::secure_erase_impl const erase;
    erase(dest, bytes);
}

}

#endif








