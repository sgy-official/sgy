

#ifndef RIPPLE_RESOURCE_ENTRY_H_INCLUDED
#define RIPPLE_RESOURCE_ENTRY_H_INCLUDED

#include <ripple/basics/DecayingSample.h>
#include <ripple/resource/impl/Key.h>
#include <ripple/resource/impl/Tuning.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/core/List.h>
#include <cassert>

namespace ripple {
namespace Resource {

using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

struct Entry
    : public beast::List <Entry>::Node
{
    Entry () = delete;

    
    explicit Entry(clock_type::time_point const now)
        : refcount (0)
        , local_balance (now)
        , remote_balance (0)
        , lastWarningTime ()
        , whenExpires ()
    {
    }

    std::string to_string() const
    {
        return key->address.to_string();
    }

    
    bool isUnlimited () const
    {
        return key->kind == kindUnlimited;
    }

    int balance (clock_type::time_point const now)
    {
        return local_balance.value (now) + remote_balance;
    }

    int add (int charge, clock_type::time_point const now)
    {
        return local_balance.add (charge, now) + remote_balance;
    }

    Key const* key;

    int refcount;

    DecayingSample <decayWindowSeconds, clock_type> local_balance;

    int remote_balance;

    clock_type::time_point lastWarningTime;

    clock_type::time_point whenExpires;
};

inline std::ostream& operator<< (std::ostream& os, Entry const& v)
{
    os << v.to_string();
    return os;
}

}
}

#endif








