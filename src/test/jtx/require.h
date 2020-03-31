

#ifndef RIPPLE_TEST_JTX_REQUIRE_H_INCLUDED
#define RIPPLE_TEST_JTX_REQUIRE_H_INCLUDED

#include <test/jtx/requires.h>
#include <functional>
#include <vector>

namespace ripple {
namespace test {
namespace jtx {

namespace detail {

inline
void
require_args (requires_t& vec)
{
}

template <class Cond, class... Args>
inline
void
require_args (requires_t& vec,
    Cond const& cond, Args const&... args)
{
    vec.push_back(cond);
    require_args(vec, args...);
}

} 


template <class...Args>
require_t
required (Args const&... args)
{
    requires_t vec;
    detail::require_args(vec, args...);
    return [vec](Env& env)
    {
        for(auto const& f : vec)
            f(env);
    };
}


class require
{
private:
    require_t cond_;

public:
    template<class... Args>
    require(Args const&... args)
        : cond_(required(args...))
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt.requires.emplace_back(cond_);
    }
};

} 
} 
} 

#endif








