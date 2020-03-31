

#include <test/jtx/deposit.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

namespace deposit {

Json::Value
auth (jtx::Account const& account, jtx::Account const& auth)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = account.human();
    jv[sfAuthorize.jsonName] = auth.human();
    jv[sfTransactionType.jsonName] = jss::DepositPreauth;
    return jv;
}

Json::Value
unauth (jtx::Account const& account, jtx::Account const& unauth)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = account.human();
    jv[sfUnauthorize.jsonName] = unauth.human();
    jv[sfTransactionType.jsonName] = jss::DepositPreauth;
    return jv;
}

} 

} 
} 
} 
























