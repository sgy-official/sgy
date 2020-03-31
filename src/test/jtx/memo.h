

#ifndef RIPPLE_TEST_JTX_MEMO_H_INCLUDED
#define RIPPLE_TEST_JTX_MEMO_H_INCLUDED

#include <test/jtx/Env.h>
#include <boost/optional.hpp>

namespace ripple {
namespace test {
namespace jtx {


class memo
{
private:
    std::string data_;
    std::string format_;
    std::string type_;

public:
    memo (std::string const& data,
        std::string const& format,
            std::string const& type)
        : data_(data)
        , format_(format)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memodata
{
private:
    std::string s_;

public:
    memodata (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memoformat
{
private:
    std::string s_;

public:
    memoformat (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memotype
{
private:
    std::string s_;

public:
    memotype (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memondata
{
private:
    std::string format_;
    std::string type_;

public:
    memondata (std::string const& format,
            std::string const& type)
        : format_(format)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memonformat
{
private:
    std::string data_;
    std::string type_;

public:
    memonformat (std::string const& data,
            std::string const& type)
        : data_(data)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memontype
{
private:
    std::string data_;
    std::string format_;

public:
    memontype (std::string const& data,
            std::string const& format)
        : data_(data)
        , format_(format)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








