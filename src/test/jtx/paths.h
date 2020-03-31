

#ifndef RIPPLE_TEST_JTX_PATHS_H_INCLUDED
#define RIPPLE_TEST_JTX_PATHS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/Issue.h>
#include <type_traits>

namespace ripple {
namespace test {
namespace jtx {


class paths
{
private:
    Issue in_;
    int depth_;
    unsigned int limit_;

public:
    paths (Issue const& in,
            int depth = 7, unsigned int limit = 4)
        : in_(in)
        , depth_(depth)
        , limit_(limit)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};



class path
{
private:
    Json::Value jv_;

public:
    path();

    template <class T, class... Args>
    explicit
    path (T const& t, Args const&... args);

    void
    operator()(Env&, JTx& jt) const;

private:
    Json::Value&
    create();

    void
    append_one(Account const& account);

    template <class T>
    std::enable_if_t<
        std::is_constructible<Account, T>::value>
    append_one(T const& t)
    {
        append_one(Account{ t });
    }

    void
    append_one(IOU const& iou);

    void
    append_one(BookSpec const& book);

    inline
    void
    append()
    {
    }

    template <class T, class... Args>
    void
    append (T const& t, Args const&... args);
};

template <class T, class... Args>
path::path (T const& t, Args const&... args)
    : jv_(Json::arrayValue)
{
    append(t, args...);
}

template <class T, class... Args>
void
path::append (T const& t, Args const&... args)
{
    append_one(t);
    append(args...);
}

} 
} 
} 

#endif








