

#include <ripple/app/paths/Pathfinder.h>
#include <test/jtx/paths.h>
#include <ripple/protocol/jss.h>

namespace ripple {
namespace test {
namespace jtx {

void
paths::operator()(Env& env, JTx& jt) const
{
    auto& jv = jt.jv;
    auto const from = env.lookup(
        jv[jss::Account].asString());
    auto const to = env.lookup(
        jv[jss::Destination].asString());
    auto const amount = amountFromJson(
        sfAmount, jv[jss::Amount]);
    Pathfinder pf (
        std::make_shared<RippleLineCache>(env.current()),
            from, to, in_.currency, in_.account,
                amount, boost::none, env.app());
    if (! pf.findPaths(depth_))
        return;

    STPath fp;
    pf.computePathRanks (limit_);
    auto const found = pf.getBestPaths(
        limit_, fp, {}, in_.account);

    if (! found.isDefault())
        jv[jss::Paths] = found.getJson(JsonOptions::none);
}


Json::Value&
path::create()
{
    return jv_.append(Json::objectValue);
}

void
path::append_one(Account const& account)
{
    auto& jv = create();
    jv["account"] = toBase58(account.id());
}

void
path::append_one(IOU const& iou)
{
    auto& jv = create();
    jv["currency"] = to_string(iou.issue().currency);
    jv["account"] = toBase58(iou.issue().account);
}

void
path::append_one(BookSpec const& book)
{
    auto& jv = create();
    jv["currency"] = to_string(book.currency);
    jv["issuer"] = toBase58(book.account);
}

void
path::operator()(Env& env, JTx& jt) const
{
    jt.jv["Paths"].append(jv_);
}

} 
} 
} 
























