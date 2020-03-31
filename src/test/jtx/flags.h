

#ifndef RIPPLE_TEST_JTX_FLAGS_H_INCLUDED
#define RIPPLE_TEST_JTX_FLAGS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/basics/contract.h>

namespace ripple {
namespace test {
namespace jtx {



Json::Value
fset (Account const& account,
    std::uint32_t on, std::uint32_t off = 0);


inline
Json::Value
fclear (Account const& account,
    std::uint32_t off)
{
    return fset(account, 0, off);
}

namespace detail {

class flags_helper
{
protected:
    std::uint32_t mask_;

private:
    inline
    void
    set_args()
    {
    }

    void
    set_args (std::uint32_t flag)
    {
        switch(flag)
        {
        case asfRequireDest:    mask_ |= lsfRequireDestTag; break;
        case asfRequireAuth:    mask_ |= lsfRequireAuth;    break;
        case asfDisallowXRP:    mask_ |= lsfDisallowXRP;    break;
        case asfDisableMaster:  mask_ |= lsfDisableMaster;  break;
        case asfNoFreeze:       mask_ |= lsfNoFreeze;       break;
        case asfGlobalFreeze:   mask_ |= lsfGlobalFreeze;   break;
        case asfDefaultRipple:  mask_ |= lsfDefaultRipple;  break;
        case asfDepositAuth:    mask_ |= lsfDepositAuth;    break;
        default:
        Throw<std::runtime_error> (
            "unknown flag");
        }
    }

    template <class Flag,
        class... Args>
    void
    set_args (std::uint32_t flag,
        Args... args)
    {
        set_args(flag, args...);
    }

protected:
    template <class... Args>
    flags_helper (Args... args)
        : mask_(0)
    {
        set_args(args...);
    }
};

} 


class flags : private detail::flags_helper
{
private:
    Account account_;

public:
    template <class... Args>
    flags (Account const& account,
            Args... args)
        : flags_helper(args...)
        , account_(account)
    {
    }

    void
    operator()(Env& env) const;
};


class nflags : private detail::flags_helper
{
private:
    Account account_;

public:
    template <class... Args>
    nflags (Account const& account,
            Args... args)
        : flags_helper(args...)
        , account_(account)
    {
    }

    void
    operator()(Env& env) const;
};

} 
} 
} 

#endif








