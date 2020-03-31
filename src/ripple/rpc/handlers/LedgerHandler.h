

#ifndef RIPPLE_RPC_HANDLERS_LEDGER_H_INCLUDED
#define RIPPLE_RPC_HANDLERS_LEDGER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/json/Object.h>
#include <ripple/protocol/jss.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/Status.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/rpc/Role.h>

namespace Json {
class Object;
}

namespace ripple {
namespace RPC {

struct Context;


class LedgerHandler {
public:
    explicit LedgerHandler (Context&);

    Status check ();

    template <class Object>
    void writeResult (Object&);

    static char const* name()
    {
        return "ledger";
    }

    static Role role()
    {
        return Role::USER;
    }

    static Condition condition()
    {
        return NO_CONDITION;
    }

private:
    Context& context_;
    std::shared_ptr<ReadView const> ledger_;
    std::vector<TxQ::TxDetails> queueTxs_;
    Json::Value result_;
    int options_ = 0;
    LedgerEntryType type_;
};


template <class Object>
void LedgerHandler::writeResult (Object& value)
{
    if (ledger_)
    {
        Json::copyFrom (value, result_);
        addJson (value, {*ledger_, options_, queueTxs_, type_});
    }
    else
    {
        auto& master = context_.app.getLedgerMaster ();
        {
            auto&& closed = Json::addObject (value, jss::closed);
            addJson (closed, {*master.getClosedLedger(), 0});
        }
        {
            auto&& open = Json::addObject (value, jss::open);
            addJson (open, {*master.getCurrentLedger(), 0});
        }
    }
}

} 
} 

#endif








