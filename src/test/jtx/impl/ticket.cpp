

#include <test/jtx/ticket.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

namespace ticket {

namespace detail {

Json::Value
create (Account const& account,
    boost::optional<Account> const& target,
        boost::optional<std::uint32_t> const& expire)
{
    Json::Value jv;
    jv[jss::Account] = account.human();
    jv[jss::TransactionType] = jss::TicketCreate;
    if (expire)
        jv["Expiration"] = *expire;
    if (target)
        jv["Target"] = target->human();
    return jv;
}

} 

Json::Value
cancel(Account const& account, std::string const & ticketId)
{
    Json::Value jv;
    jv[jss::TransactionType] = jss::TicketCancel;
    jv[jss::Account] = account.human();
    jv["TicketID"] = ticketId;
    return jv;
}

} 

} 
} 
} 
























