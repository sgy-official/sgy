

#ifndef RIPPLE_TEST_ABSTRACTCLIENT_H_INCLUDED
#define RIPPLE_TEST_ABSTRACTCLIENT_H_INCLUDED

#include <ripple/json/json_value.h>

namespace ripple {
namespace test {


class AbstractClient
{
public:
    virtual ~AbstractClient() = default;
    AbstractClient() = default;
    AbstractClient(AbstractClient const&) = delete;
    AbstractClient& operator=(AbstractClient const&) = delete;

    
    virtual
    Json::Value
    invoke(std::string const& cmd,
        Json::Value const& params = {}) = 0;

    virtual unsigned version() const = 0;
};

} 
} 

#endif








