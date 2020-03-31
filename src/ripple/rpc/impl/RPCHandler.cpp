

#include <ripple/app/main/Application.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/core/Config.h>
#include <ripple/core/JobQueue.h>
#include <ripple/json/Object.h>
#include <ripple/json/to_string.h>
#include <ripple/net/InfoSub.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/jss.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Role.h>
#include <ripple/resource/Fees.h>
#include <atomic>
#include <chrono>

namespace ripple {
namespace RPC {

namespace {



error_code_i fillHandler (Context& context,
                          Handler const * & result)
{
    if (! isUnlimited (context.role))
    {
        int jc = context.app.getJobQueue ().getJobCountGE (jtCLIENT);
        if (jc > Tuning::maxJobQueueClients)
        {
            JLOG (context.j.debug()) << "Too busy for command: " << jc;
            return rpcTOO_BUSY;
        }
    }

    if (!context.params.isMember(jss::command) && !context.params.isMember(jss::method))
        return rpcCOMMAND_MISSING;
    if (context.params.isMember(jss::command) && context.params.isMember(jss::method))
    {
        if (context.params[jss::command].asString() !=
            context.params[jss::method].asString())
            return rpcUNKNOWN_COMMAND;
    }

    std::string strCommand  = context.params.isMember(jss::command) ?
                              context.params[jss::command].asString() :
                              context.params[jss::method].asString();

    JLOG (context.j.trace()) << "COMMAND:" << strCommand;
    JLOG (context.j.trace()) << "REQUEST:" << context.params;
    auto handler = getHandler(strCommand);

    if (!handler)
        return rpcUNKNOWN_COMMAND;

    if (handler->role_ == Role::ADMIN && context.role != Role::ADMIN)
        return rpcNO_PERMISSION;

    if ((handler->condition_ & NEEDS_NETWORK_CONNECTION) &&
        (context.netOps.getOperatingMode () < NetworkOPs::omSYNCING))
    {
        JLOG (context.j.info())
            << "Insufficient network mode for RPC: "
            << context.netOps.strOperatingMode ();

        return rpcNO_NETWORK;
    }

    if (context.app.getOPs().isAmendmentBlocked() &&
         (handler->condition_ & NEEDS_CURRENT_LEDGER ||
          handler->condition_ & NEEDS_CLOSED_LEDGER))
    {
        return rpcAMENDMENT_BLOCKED;
    }

    if (!context.app.config().standalone() &&
        handler->condition_ & NEEDS_CURRENT_LEDGER)
    {
        if (context.ledgerMaster.getValidatedLedgerAge () >
            Tuning::maxValidatedLedgerAge)
        {
            return rpcNO_CURRENT;
        }

        auto const cID = context.ledgerMaster.getCurrentLedgerIndex ();
        auto const vID = context.ledgerMaster.getValidLedgerIndex ();

        if (cID + 10 < vID)
        {
            JLOG (context.j.debug()) << "Current ledger ID(" << cID <<
                ") is less than validated ledger ID(" << vID << ")";
            return rpcNO_CURRENT;
        }
    }

    if ((handler->condition_ & NEEDS_CLOSED_LEDGER) &&
        !context.ledgerMaster.getClosedLedger ())
    {
        return rpcNO_CLOSED;
    }

    result = handler;
    return rpcSUCCESS;
}

template <class Object, class Method>
Status callMethod (
    Context& context, Method method, std::string const& name, Object& result)
{
    static std::atomic<std::uint64_t> requestId {0};
    auto& perfLog = context.app.getPerfLog();
    std::uint64_t const curId = ++requestId;
    try
    {
        perfLog.rpcStart(name, curId);
        auto v = context.app.getJobQueue().makeLoadEvent(
            jtGENERIC, "cmd:" + name);

        auto ret = method (context, result);
        perfLog.rpcFinish(name, curId);
        return ret;
    }
    catch (std::exception& e)
    {
        perfLog.rpcError(name, curId);
        JLOG (context.j.info()) << "Caught throw: " << e.what ();

        if (context.loadType == Resource::feeReferenceRPC)
            context.loadType = Resource::feeExceptionRPC;

        inject_error (rpcINTERNAL, result);
        return rpcINTERNAL;
    }
}

template <class Method, class Object>
void getResult (
    Context& context, Method method, Object& object, std::string const& name)
{
    auto&& result = Json::addObject (object, jss::result);
    if (auto status = callMethod (context, method, name, result))
    {
        JLOG (context.j.debug()) << "rpcError: " << status.toString();
        result[jss::status] = jss::error;

        auto rq = context.params;

        if (rq.isObject())
        {
            if (rq.isMember(jss::passphrase.c_str()))
                rq[jss::passphrase.c_str()] = "<masked>";
            if (rq.isMember(jss::secret.c_str()))
                rq[jss::secret.c_str()] = "<masked>";
            if (rq.isMember(jss::seed.c_str()))
                rq[jss::seed.c_str()] = "<masked>";
            if (rq.isMember(jss::seed_hex.c_str()))
                rq[jss::seed_hex.c_str()] = "<masked>";
        }

        result[jss::request] = rq;
    }
    else
    {
        result[jss::status] = jss::success;
    }
}

} 

Status doCommand (
    RPC::Context& context, Json::Value& result)
{
    Handler const * handler = nullptr;
    if (auto error = fillHandler (context, handler))
    {
        inject_error (error, result);
        return error;
    }

    if (auto method = handler->valueMethod_)
    {
        if (! context.headers.user.empty() ||
            ! context.headers.forwardedFor.empty())
        {
            JLOG(context.j.debug()) << "start command: " << handler->name_ <<
                ", user: " << context.headers.user << ", forwarded for: " <<
                    context.headers.forwardedFor;

            auto ret = callMethod (context, method, handler->name_, result);

            JLOG(context.j.debug()) << "finish command: " << handler->name_ <<
                ", user: " << context.headers.user << ", forwarded for: " <<
                    context.headers.forwardedFor;

            return ret;
        }
        else
        {
            return callMethod (context, method, handler->name_, result);
        }
    }

    return rpcUNKNOWN_COMMAND;
}

Role roleRequired (std::string const& method)
{
    auto handler = RPC::getHandler(method);

    if (!handler)
        return Role::FORBID;

    return handler->role_;
}

} 
} 
























