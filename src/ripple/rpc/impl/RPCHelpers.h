

#ifndef RIPPLE_RPC_RPCHELPERS_H_INCLUDED
#define RIPPLE_RPC_RPCHELPERS_H_INCLUDED

#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/ledger/TxMeta.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/Status.h>
#include <boost/optional.hpp>

namespace Json {
class Value;
}

namespace ripple {

class ReadView;
class Transaction;

namespace RPC {

struct Context;


boost::optional<AccountID>
accountFromStringStrict (std::string const&);

Json::Value
accountFromString (AccountID& result, std::string const& strIdent,
    bool bStrict = false);


bool
getAccountObjects (ReadView const& ledger, AccountID const& account,
    LedgerEntryType const type, uint256 dirIndex, uint256 const& entryIndex,
    std::uint32_t const limit, Json::Value& jvResult);


Json::Value
lookupLedger (std::shared_ptr<ReadView const>&, Context&);


Status
lookupLedger (std::shared_ptr<ReadView const>&, Context&, Json::Value& result);

hash_set <AccountID>
parseAccountIds(Json::Value const& jvArray);


void
injectSLE(Json::Value& jv, SLE const& sle);


boost::optional<Json::Value>
readLimitField(unsigned int& limit, Tuning::LimitRange const&, Context const&);

boost::optional<Seed>
getSeedFromRPC(Json::Value const& params, Json::Value& error);

boost::optional<Seed>
parseRippleLibSeed(Json::Value const& params);

std::pair<PublicKey, SecretKey>
keypairForSignature(Json::Value const& params, Json::Value& error);

extern beast::SemanticVersion const firstVersion;
extern beast::SemanticVersion const goodVersion;
extern beast::SemanticVersion const lastVersion;

template <class Object>
void
setVersion(Object& parent)
{
    auto&& object = addObject (parent, jss::version);
    object[jss::first] = firstVersion.print();
    object[jss::good] = goodVersion.print();
    object[jss::last] = lastVersion.print();
}

std::pair<RPC::Status, LedgerEntryType>
    chooseLedgerEntryType(Json::Value const& params);

} 
} 

#endif








