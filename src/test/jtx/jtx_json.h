

#ifndef RIPPLE_TEST_JTX_JSON_H_INCLUDED
#define RIPPLE_TEST_JTX_JSON_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/json/json_value.h>

namespace ripple {
namespace test {
namespace jtx {


class json
{
private:
    Json::Value jv_;

public:
    explicit
    json (std::string const&);

    explicit
    json (char const*);

    explicit
    json (Json::Value);

    template <class T>
    json (Json::StaticString const& key, T const& value)
    {
        jv_[key] = value;
    }

    template <class T>
    json (std::string const& key, T const& value)
    {
        jv_[key] = value;
    }

    void
    operator()(Env&, JTx& jt) const;
};

} 
} 
} 

#endif








