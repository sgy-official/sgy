

#include <test/jtx/utility.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/STParsedJSON.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/basics/contract.h>
#include <cstring>

namespace ripple {
namespace test {
namespace jtx {

STObject
parse (Json::Value const& jv)
{
    STParsedJSONObject p("tx_json", jv);
    if (! p.object)
        Throw<parse_error> (
            rpcErrorString(p.error));
    return std::move(*p.object);
}

void
sign (Json::Value& jv,
    Account const& account)
{
    jv[jss::SigningPubKey] =
        strHex(account.pk().slice());
    Serializer ss;
    ss.add32 (HashPrefix::txSign);
    parse(jv).addWithoutSigningFields(ss);
    auto const sig = ripple::sign(
        account.pk(), account.sk(), ss.slice());
    jv[jss::TxnSignature] =
        strHex(Slice{ sig.data(), sig.size() });
}

void
fill_fee (Json::Value& jv,
    ReadView const& view)
{
    if (jv.isMember(jss::Fee))
        return;
    jv[jss::Fee] = std::to_string(
        view.fees().base);
}

void
fill_seq (Json::Value& jv,
    ReadView const& view)
{
    if (jv.isMember(jss::Sequence))
        return;
    auto const account =
        parseBase58<AccountID>(
            jv[jss::Account].asString());
    if (! account)
        Throw<parse_error> (
            "unexpected invalid Account");
    auto const ar = view.read(
        keylet::account(*account));
    if (! ar)
        Throw<parse_error> (
            "unexpected missing account root");
    jv[jss::Sequence] =
        ar->getFieldU32(sfSequence);
}

} 
} 
} 
























