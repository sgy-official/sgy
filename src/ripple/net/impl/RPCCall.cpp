

#include <ripple/app/main/Application.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/net/RPCCall.h>
#include <ripple/net/RPCErr.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/json/Object.h>
#include <ripple/net/HTTPClient.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/rpc/ServerHandler.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/beast/core/string.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <array>
#include <iostream>
#include <type_traits>
#include <unordered_map>

namespace ripple {

class RPCParser;


std::string createHTTPPost (
    std::string const& strHost,
    std::string const& strPath,
    std::string const& strMsg,
    std::unordered_map<std::string, std::string> const& mapRequestHeaders)
{
    std::ostringstream s;


    s << "POST "
      << (strPath.empty () ? "/" : strPath)
      << " HTTP/1.0\r\n"
      << "User-Agent: " << systemName () << "-json-rpc/v1\r\n"
      << "Host: " << strHost << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << strMsg.size () << "\r\n"
      << "Accept: application/json\r\n";

    for (auto const& item : mapRequestHeaders)
        s << item.first << ": " << item.second << "\r\n";

    s << "\r\n" << strMsg;

    return s.str ();
}

class RPCParser
{
private:
    beast::Journal j_;

    static bool jvParseLedger (Json::Value& jvRequest, std::string const& strLedger)
    {
        if (strLedger == "current" || strLedger == "closed" || strLedger == "validated")
        {
            jvRequest[jss::ledger_index]   = strLedger;
        }
        else if (strLedger.length () == 64)
        {
            jvRequest[jss::ledger_hash]    = strLedger;
        }
        else
        {
            jvRequest[jss::ledger_index]   = beast::lexicalCast <std::uint32_t> (strLedger);
        }

        return true;
    }

    static Json::Value jvParseCurrencyIssuer (std::string const& strCurrencyIssuer)
    {
        static boost::regex reCurIss ("\\`([[:alpha:]]{3})(?:/(.+))?\\'");

        boost::smatch   smMatch;

        if (boost::regex_match (strCurrencyIssuer, smMatch, reCurIss))
        {
            Json::Value jvResult (Json::objectValue);
            std::string strCurrency = smMatch[1];
            std::string strIssuer   = smMatch[2];

            jvResult[jss::currency]    = strCurrency;

            if (strIssuer.length ())
            {
                jvResult[jss::issuer]      = strIssuer;
            }

            return jvResult;
        }
        else
        {
            return RPC::make_param_error (std::string ("Invalid currency/issuer '") +
                    strCurrencyIssuer + "'");
        }
    }

private:
    using parseFuncPtr = Json::Value (RPCParser::*) (Json::Value const& jvParams);

    Json::Value parseAsIs (Json::Value const& jvParams)
    {
        Json::Value v (Json::objectValue);

        if (jvParams.isArray() && (jvParams.size () > 0))
            v[jss::params] = jvParams;

        return v;
    }

    Json::Value parseDownloadShard(Json::Value const& jvParams)
    {
        Json::Value jvResult(Json::objectValue);
        unsigned int sz {jvParams.size()};
        unsigned int i {0};

        if (sz & 1)
        {
            using namespace boost::beast::detail;
            if (iequals(jvParams[0u].asString(), "novalidate"))
                ++i;
            else if (!iequals(jvParams[--sz].asString(), "novalidate"))
                return rpcError(rpcINVALID_PARAMS);
            jvResult[jss::validate] = false;
        }

        Json::Value shards(Json::arrayValue);
        for (; i < sz; i += 2)
        {
            Json::Value shard(Json::objectValue);
            shard[jss::index] = jvParams[i].asUInt();
            shard[jss::url] = jvParams[i + 1].asString();
            shards.append(std::move(shard));
        }
        jvResult[jss::shards] = std::move(shards);

        return jvResult;
    }

    Json::Value parseInternal (Json::Value const& jvParams)
    {
        Json::Value v (Json::objectValue);
        v[jss::internal_command] = jvParams[0u];

        Json::Value params (Json::arrayValue);

        for (unsigned i = 1; i < jvParams.size (); ++i)
            params.append (jvParams[i]);

        v[jss::params] = params;

        return v;
    }

    Json::Value parseFetchInfo (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        if (iParams != 0)
            jvRequest[jvParams[0u].asString()] = true;

        return jvRequest;
    }

    Json::Value
    parseAccountTransactions (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        auto const account =
            parseBase58<AccountID>(jvParams[0u].asString());
        if (! account)
            return rpcError (rpcACT_MALFORMED);

        jvRequest[jss::account]= toBase58(*account);

        bool            bDone   = false;

        while (!bDone && iParams >= 2)
        {
            if (jvParams[iParams - 1].asString () == jss::binary)
            {
                jvRequest[jss::binary]     = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::count)
            {
                jvRequest[jss::count]      = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::descending)
            {
                jvRequest[jss::descending] = true;
                --iParams;
            }
            else
            {
                bDone   = true;
            }
        }

        if (1 == iParams)
        {
        }
        else if (2 == iParams)
        {
            if (!jvParseLedger (jvRequest, jvParams[1u].asString ()))
                return jvRequest;
        }
        else
        {
            std::int64_t   uLedgerMin  = jvParams[1u].asInt ();
            std::int64_t   uLedgerMax  = jvParams[2u].asInt ();

            if (uLedgerMax != -1 && uLedgerMax < uLedgerMin)
            {
                return rpcError (rpcLGR_IDXS_INVALID);
            }

            jvRequest[jss::ledger_index_min]   = jvParams[1u].asInt ();
            jvRequest[jss::ledger_index_max]   = jvParams[2u].asInt ();

            if (iParams >= 4)
                jvRequest[jss::limit]  = jvParams[3u].asInt ();

            if (iParams >= 5)
                jvRequest[jss::offset] = jvParams[4u].asInt ();
        }

        return jvRequest;
    }

    Json::Value parseTxAccount (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        auto const account =
            parseBase58<AccountID>(jvParams[0u].asString());
        if (! account)
            return rpcError (rpcACT_MALFORMED);

        jvRequest[jss::account]    = toBase58(*account);

        bool            bDone   = false;

        while (!bDone && iParams >= 2)
        {
            if (jvParams[iParams - 1].asString () == jss::binary)
            {
                jvRequest[jss::binary]     = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::count)
            {
                jvRequest[jss::count]      = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::forward)
            {
                jvRequest[jss::forward] = true;
                --iParams;
            }
            else
            {
                bDone   = true;
            }
        }

        if (1 == iParams)
        {
        }
        else if (2 == iParams)
        {
            if (!jvParseLedger (jvRequest, jvParams[1u].asString ()))
                return jvRequest;
        }
        else
        {
            std::int64_t   uLedgerMin  = jvParams[1u].asInt ();
            std::int64_t   uLedgerMax  = jvParams[2u].asInt ();

            if (uLedgerMax != -1 && uLedgerMax < uLedgerMin)
            {
                return rpcError (rpcLGR_IDXS_INVALID);
            }

            jvRequest[jss::ledger_index_min]   = jvParams[1u].asInt ();
            jvRequest[jss::ledger_index_max]   = jvParams[2u].asInt ();

            if (iParams >= 4)
                jvRequest[jss::limit]  = jvParams[3u].asInt ();
        }

        return jvRequest;
    }

    Json::Value parseBookOffers (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        Json::Value     jvTakerPays = jvParseCurrencyIssuer (jvParams[0u].asString ());
        Json::Value     jvTakerGets = jvParseCurrencyIssuer (jvParams[1u].asString ());

        if (isRpcError (jvTakerPays))
        {
            return jvTakerPays;
        }
        else
        {
            jvRequest[jss::taker_pays] = jvTakerPays;
        }

        if (isRpcError (jvTakerGets))
        {
            return jvTakerGets;
        }
        else
        {
            jvRequest[jss::taker_gets] = jvTakerGets;
        }

        if (jvParams.size () >= 3)
        {
            jvRequest[jss::issuer] = jvParams[2u].asString ();
        }

        if (jvParams.size () >= 4 && !jvParseLedger (jvRequest, jvParams[3u].asString ()))
            return jvRequest;

        if (jvParams.size () >= 5)
        {
            int     iLimit  = jvParams[5u].asInt ();

            if (iLimit > 0)
                jvRequest[jss::limit]  = iLimit;
        }

        if (jvParams.size () >= 6 && jvParams[5u].asInt ())
        {
            jvRequest[jss::proof]  = true;
        }

        if (jvParams.size () == 7)
            jvRequest[jss::marker] = jvParams[6u];

        return jvRequest;
    }

    Json::Value parseCanDelete (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (!jvParams.size ())
            return jvRequest;

        std::string input = jvParams[0u].asString();
        if (input.find_first_not_of("0123456789") ==
                std::string::npos)
            jvRequest["can_delete"] = jvParams[0u].asUInt();
        else
            jvRequest["can_delete"] = input;

        return jvRequest;
    }

    Json::Value parseConnect (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        jvRequest[jss::ip] = jvParams[0u].asString ();

        if (jvParams.size () == 2)
            jvRequest[jss::port]   = jvParams[1u].asUInt ();

        return jvRequest;
    }

    Json::Value parseDepositAuthorized (Json::Value const& jvParams)
    {
        Json::Value jvRequest (Json::objectValue);
        jvRequest[jss::source_account] = jvParams[0u].asString ();
        jvRequest[jss::destination_account] = jvParams[1u].asString ();

        if (jvParams.size () == 3)
            jvParseLedger (jvRequest, jvParams[2u].asString ());

        return jvRequest;
    }

    Json::Value parseEvented (Json::Value const& jvParams)
    {
        return rpcError (rpcNO_EVENTS);
    }

    Json::Value parseFeature (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size () > 0)
            jvRequest[jss::feature] = jvParams[0u].asString ();

        if (jvParams.size () > 1)
        {
            auto const action = jvParams[1u].asString ();

            if (boost::beast::detail::iequals(action, "reject"))
                jvRequest[jss::vetoed] = Json::Value (true);
            else if (boost::beast::detail::iequals(action, "accept"))
                jvRequest[jss::vetoed] = Json::Value (false);
            else
                return rpcError (rpcINVALID_PARAMS);
        }

        return jvRequest;
    }

    Json::Value parseGetCounts (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size ())
            jvRequest[jss::min_count]  = jvParams[0u].asUInt ();

        return jvRequest;
    }

    Json::Value parseSignFor (Json::Value const& jvParams)
    {
        bool const bOffline = 4 == jvParams.size () && jvParams[3u].asString () == "offline";

        if (3 == jvParams.size () || bOffline)
        {
            Json::Value txJSON;
            Json::Reader reader;
            if (reader.parse (jvParams[2u].asString (), txJSON))
            {
                Json::Value jvRequest{Json::objectValue};

                jvRequest[jss::account] = jvParams[0u].asString ();
                jvRequest[jss::secret]  = jvParams[1u].asString ();
                jvRequest[jss::tx_json] = txJSON;

                if (bOffline)
                    jvRequest[jss::offline] = true;

                return jvRequest;
            }
        }
        return rpcError (rpcINVALID_PARAMS);
    }

    Json::Value parseJson (Json::Value const& jvParams)
    {
        Json::Reader    reader;
        Json::Value     jvRequest;

        JLOG (j_.trace()) << "RPC method: " << jvParams[0u];
        JLOG (j_.trace()) << "RPC json: " << jvParams[1u];

        if (reader.parse (jvParams[1u].asString (), jvRequest))
        {
            if (!jvRequest.isObjectOrNull ())
                return rpcError (rpcINVALID_PARAMS);

            jvRequest[jss::method] = jvParams[0u];

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

    bool isValidJson2(Json::Value const& jv)
    {
        if (jv.isArray())
        {
            if (jv.size() == 0)
                return false;
            for (auto const& j : jv)
            {
                if (!isValidJson2(j))
                    return false;
            }
            return true;
        }
        if (jv.isObject())
        {
            if (jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0" &&
                jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0" &&
                jv.isMember(jss::id) && jv.isMember(jss::method))
            {
                if (jv.isMember(jss::params) &&
                      !(jv[jss::params].isNull() || jv[jss::params].isArray() ||
                                                    jv[jss::params].isObject()))
                    return false;
                return true;
            }
        }
        return false;
    }

    Json::Value parseJson2(Json::Value const& jvParams)
    {
        Json::Reader reader;
        Json::Value jv;
        bool valid_parse = reader.parse(jvParams[0u].asString(), jv);
        if (valid_parse && isValidJson2(jv))
        {
            if (jv.isObject())
            {
                Json::Value jv1{Json::objectValue};
                if (jv.isMember(jss::params))
                {
                    auto const& params = jv[jss::params];
                    for (auto i = params.begin(); i != params.end(); ++i)
                        jv1[i.key().asString()] = *i;
                }
                jv1[jss::jsonrpc] = jv[jss::jsonrpc];
                jv1[jss::ripplerpc] = jv[jss::ripplerpc];
                jv1[jss::id] = jv[jss::id];
                jv1[jss::method] = jv[jss::method];
                return jv1;
            }
            Json::Value jv1{Json::arrayValue};
            for (Json::UInt j = 0; j < jv.size(); ++j)
            {
                if (jv[j].isMember(jss::params))
                {
                    auto const& params = jv[j][jss::params];
                    for (auto i = params.begin(); i != params.end(); ++i)
                        jv1[j][i.key().asString()] = *i;
                }
                jv1[j][jss::jsonrpc] = jv[j][jss::jsonrpc];
                jv1[j][jss::ripplerpc] = jv[j][jss::ripplerpc];
                jv1[j][jss::id] = jv[j][jss::id];
                jv1[j][jss::method] = jv[j][jss::method];
            }
            return jv1;
        }
        auto jv_error = rpcError(rpcINVALID_PARAMS);
        if (jv.isMember(jss::jsonrpc))
            jv_error[jss::jsonrpc] = jv[jss::jsonrpc];
        if (jv.isMember(jss::ripplerpc))
            jv_error[jss::ripplerpc] = jv[jss::ripplerpc];
        if (jv.isMember(jss::id))
            jv_error[jss::id] = jv[jss::id];
        return jv_error;
    }

    Json::Value parseLedger (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (!jvParams.size ())
        {
            return jvRequest;
        }

        jvParseLedger (jvRequest, jvParams[0u].asString ());

        if (2 == jvParams.size ())
        {
            if (jvParams[1u].asString () == "full")
            {
                jvRequest[jss::full]   = true;
            }
            else if (jvParams[1u].asString () == "tx")
            {
                jvRequest[jss::transactions] = true;
                jvRequest[jss::expand] = true;
            }
        }

        return jvRequest;
    }

    Json::Value parseLedgerId (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        std::string     strLedger   = jvParams[0u].asString ();

        if (strLedger.length () == 64)
        {
            jvRequest[jss::ledger_hash]    = strLedger;
        }
        else
        {
            jvRequest[jss::ledger_index]   = beast::lexicalCast <std::uint32_t> (strLedger);
        }

        return jvRequest;
    }

    Json::Value parseLogLevel (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size () == 1)
        {
            jvRequest[jss::severity] = jvParams[0u].asString ();
        }
        else if (jvParams.size () == 2)
        {
            jvRequest[jss::partition] = jvParams[0u].asString ();
            jvRequest[jss::severity] = jvParams[1u].asString ();
        }

        return jvRequest;
    }

    Json::Value parseAccountItems (Json::Value const& jvParams)
    {
        return parseAccountRaw1 (jvParams);
    }

    Json::Value parseAccountCurrencies (Json::Value const& jvParams)
    {
        return parseAccountRaw1 (jvParams);
    }

    Json::Value parseAccountLines (Json::Value const& jvParams)
    {
        return parseAccountRaw2 (jvParams, jss::peer);
    }

    Json::Value parseAccountChannels (Json::Value const& jvParams)
    {
        return parseAccountRaw2 (jvParams, jss::destination_account);
    }

    Json::Value parseChannelAuthorize (Json::Value const& jvParams)
    {
        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::secret] = jvParams[0u];
        {
            uint256 channelId;
            if (!channelId.SetHexExact (jvParams[1u].asString ()))
                return rpcError (rpcCHANNEL_MALFORMED);
        }
        jvRequest[jss::channel_id] = jvParams[1u].asString ();

        if (!jvParams[2u].isString() || !to_uint64(jvParams[2u].asString()))
            return rpcError(rpcCHANNEL_AMT_MALFORMED);
        jvRequest[jss::amount] = jvParams[2u];

        return jvRequest;
    }

    Json::Value parseChannelVerify (Json::Value const& jvParams)
    {
        std::string const strPk = jvParams[0u].asString ();

        bool const validPublicKey = [&strPk]{
            if (parseBase58<PublicKey> (TokenType::AccountPublic, strPk))
                return true;

            std::pair<Blob, bool> pkHex(strUnHex (strPk));
            if (!pkHex.second)
                return false;

            if (!publicKeyType(makeSlice(pkHex.first)))
                return false;

            return true;
        }();

        if (!validPublicKey)
            return rpcError (rpcPUBLIC_MALFORMED);

        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::public_key] = strPk;
        {
            uint256 channelId;
            if (!channelId.SetHexExact (jvParams[1u].asString ()))
                return rpcError (rpcCHANNEL_MALFORMED);
        }
        jvRequest[jss::channel_id] = jvParams[1u].asString ();

        if (!jvParams[2u].isString() || !to_uint64(jvParams[2u].asString()))
            return rpcError(rpcCHANNEL_AMT_MALFORMED);
        jvRequest[jss::amount] = jvParams[2u];

        jvRequest[jss::signature] = jvParams[3u].asString ();

        return jvRequest;
    }

    Json::Value parseAccountRaw2 (Json::Value const& jvParams,
                                  char const * const acc2Field)
    {
        std::array<char const* const, 2> accFields{{jss::account, acc2Field}};
        auto const nParams = jvParams.size ();
        Json::Value jvRequest (Json::objectValue);
        for (auto i = 0; i < nParams; ++i)
        {
            std::string strParam = jvParams[i].asString ();

            if (i==1 && strParam.empty())
                continue;

            if (i < 2)
            {
                if (parseBase58<PublicKey> (
                        TokenType::AccountPublic, strParam) ||
                    parseBase58<AccountID> (strParam) ||
                    parseGenericSeed (strParam))
                {
                    jvRequest[accFields[i]] = std::move (strParam);
                }
                else
                {
                    return rpcError (rpcACT_MALFORMED);
                }
            }
            else
            {
                if (jvParseLedger (jvRequest, strParam))
                    return jvRequest;
                return rpcError (rpcLGR_IDX_MALFORMED);
            }
        }

        return jvRequest;
    }

    Json::Value parseAccountRaw1 (Json::Value const& jvParams)
    {
        std::string     strIdent    = jvParams[0u].asString ();
        unsigned int    iCursor     = jvParams.size ();
        bool            bStrict     = false;

        if (iCursor >= 2 && jvParams[iCursor - 1] == jss::strict)
        {
            bStrict = true;
            --iCursor;
        }

        if (! parseBase58<PublicKey>(TokenType::AccountPublic, strIdent) &&
            ! parseBase58<AccountID>(strIdent) &&
            ! parseGenericSeed(strIdent))
            return rpcError (rpcACT_MALFORMED);

        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::account]    = strIdent;

        if (bStrict)
            jvRequest[jss::strict]     = 1;

        if (iCursor == 2 && !jvParseLedger (jvRequest, jvParams[1u].asString ()))
            return rpcError (rpcLGR_IDX_MALFORMED);

        return jvRequest;
    }

    Json::Value parseRipplePathFind (Json::Value const& jvParams)
    {
        Json::Reader    reader;
        Json::Value     jvRequest{Json::objectValue};
        bool            bLedger     = 2 == jvParams.size ();

        JLOG (j_.trace()) << "RPC json: " << jvParams[0u];

        if (reader.parse (jvParams[0u].asString (), jvRequest))
        {
            if (bLedger)
            {
                jvParseLedger (jvRequest, jvParams[1u].asString ());
            }

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

    Json::Value parseSignSubmit (Json::Value const& jvParams)
    {
        Json::Value     txJSON;
        Json::Reader    reader;
        bool const      bOffline    = 3 == jvParams.size () && jvParams[2u].asString () == "offline";

        if (1 == jvParams.size ())
        {

            Json::Value jvRequest{Json::objectValue};

            jvRequest[jss::tx_blob]    = jvParams[0u].asString ();

            return jvRequest;
        }
        else if ((2 == jvParams.size () || bOffline)
                 && reader.parse (jvParams[1u].asString (), txJSON))
        {
            Json::Value jvRequest{Json::objectValue};

            jvRequest[jss::secret]     = jvParams[0u].asString ();
            jvRequest[jss::tx_json]    = txJSON;

            if (bOffline)
                jvRequest[jss::offline]    = true;

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

    Json::Value parseSubmitMultiSigned (Json::Value const& jvParams)
    {
        if (1 == jvParams.size ())
        {
            Json::Value     txJSON;
            Json::Reader    reader;
            if (reader.parse (jvParams[0u].asString (), txJSON))
            {
                Json::Value jvRequest{Json::objectValue};
                jvRequest[jss::tx_json] = txJSON;
                return jvRequest;
            }
        }

        return rpcError (rpcINVALID_PARAMS);
    }

    Json::Value parseTransactionEntry (Json::Value const& jvParams)
    {
        assert (jvParams.size() == 2);

        std::string const txHash = jvParams[0u].asString();
        if (txHash.length() != 64)
            return rpcError (rpcINVALID_PARAMS);

        Json::Value jvRequest{Json::objectValue};
        jvRequest[jss::tx_hash] = txHash;

        jvParseLedger (jvRequest, jvParams[1u].asString());

        if (jvRequest.isMember(jss::ledger_index) &&
            jvRequest[jss::ledger_index] == 0)
                return rpcError (rpcINVALID_PARAMS);

        return jvRequest;
    }


    Json::Value parseTx (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size () > 1)
        {
            if (jvParams[1u].asString () == jss::binary)
                jvRequest[jss::binary] = true;
        }

        jvRequest["transaction"]    = jvParams[0u].asString ();
        return jvRequest;
    }

    Json::Value parseTxHistory (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        jvRequest[jss::start]  = jvParams[0u].asUInt ();

        return jvRequest;
    }

    Json::Value parseValidationCreate (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size ())
            jvRequest[jss::secret]     = jvParams[0u].asString ();

        return jvRequest;
    }

    Json::Value parseWalletPropose (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size ())
            jvRequest[jss::passphrase]     = jvParams[0u].asString ();

        return jvRequest;
    }


    Json::Value parseGatewayBalances (Json::Value const& jvParams)
    {
        unsigned int index = 0;
        const unsigned int size = jvParams.size ();

        Json::Value jvRequest{Json::objectValue};

        std::string param = jvParams[index++].asString ();
        if (param.empty ())
            return RPC::make_param_error ("Invalid first parameter");

        if (param[0] != 'r')
        {
            if (param.size() == 64)
                jvRequest[jss::ledger_hash] = param;
            else
                jvRequest[jss::ledger_index] = param;

            if (size <= index)
                return RPC::make_param_error ("Invalid hotwallet");

            param = jvParams[index++].asString ();
        }

        jvRequest[jss::account] = param;

        if (index < size)
        {
            Json::Value& hotWallets =
                (jvRequest["hotwallet"] = Json::arrayValue);
            while (index < size)
                hotWallets.append (jvParams[index++].asString ());
        }

        return jvRequest;
    }

    Json::Value parseServerInfo (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        if (jvParams.size() == 1 && jvParams[0u].asString() == "counters")
            jvRequest[jss::counters] = true;
        return jvRequest;
    }

public:

    explicit
    RPCParser (beast::Journal j)
            :j_ (j){}


    Json::Value parseCommand (std::string strMethod, Json::Value jvParams, bool allowAnyCommand)
    {
        if (auto stream = j_.trace())
        {
            stream << "Method: '" << strMethod << "'";
            stream << "Params: " << jvParams;
        }

        struct Command
        {
            const char*     name;
            parseFuncPtr    parse;
            int             minParams;
            int             maxParams;
        };

        static
        Command const commands[] =
        {
            {   "account_currencies",   &RPCParser::parseAccountCurrencies,     1,  2   },
            {   "account_info",         &RPCParser::parseAccountItems,          1,  2   },
            {   "account_lines",        &RPCParser::parseAccountLines,          1,  5   },
            {   "account_channels",     &RPCParser::parseAccountChannels,       1,  3   },
            {   "account_objects",      &RPCParser::parseAccountItems,          1,  5   },
            {   "account_offers",       &RPCParser::parseAccountItems,          1,  4   },
            {   "account_tx",           &RPCParser::parseAccountTransactions,   1,  8   },
            {   "book_offers",          &RPCParser::parseBookOffers,            2,  7   },
            {   "can_delete",           &RPCParser::parseCanDelete,             0,  1   },
            {   "channel_authorize",    &RPCParser::parseChannelAuthorize,      3,  3   },
            {   "channel_verify",       &RPCParser::parseChannelVerify,         4,  4   },
            {   "connect",              &RPCParser::parseConnect,               1,  2   },
            {   "consensus_info",       &RPCParser::parseAsIs,                  0,  0   },
            {   "deposit_authorized",   &RPCParser::parseDepositAuthorized,     2,  3   },
            {   "download_shard",       &RPCParser::parseDownloadShard,         2,  -1  },
            {   "feature",              &RPCParser::parseFeature,               0,  2   },
            {   "fetch_info",           &RPCParser::parseFetchInfo,             0,  1   },
            {   "gateway_balances",     &RPCParser::parseGatewayBalances,       1, -1   },
            {   "get_counts",           &RPCParser::parseGetCounts,             0,  1   },
            {   "json",                 &RPCParser::parseJson,                  2,  2   },
            {   "json2",                &RPCParser::parseJson2,                 1,  1   },
            {   "ledger",               &RPCParser::parseLedger,                0,  2   },
            {   "ledger_accept",        &RPCParser::parseAsIs,                  0,  0   },
            {   "ledger_closed",        &RPCParser::parseAsIs,                  0,  0   },
            {   "ledger_current",       &RPCParser::parseAsIs,                  0,  0   },
            {   "ledger_header",        &RPCParser::parseLedgerId,              1,  1   },
            {   "ledger_request",       &RPCParser::parseLedgerId,              1,  1   },
            {   "log_level",            &RPCParser::parseLogLevel,              0,  2   },
            {   "logrotate",            &RPCParser::parseAsIs,                  0,  0   },
            {   "owner_info",           &RPCParser::parseAccountItems,          1,  2   },
            {   "peers",                &RPCParser::parseAsIs,                  0,  0   },
            {   "ping",                 &RPCParser::parseAsIs,                  0,  0   },
            {   "print",                &RPCParser::parseAsIs,                  0,  1   },
            {   "random",               &RPCParser::parseAsIs,                  0,  0   },
            {   "ripple_path_find",     &RPCParser::parseRipplePathFind,        1,  2   },
            {   "sign",                 &RPCParser::parseSignSubmit,            2,  3   },
            {   "sign_for",             &RPCParser::parseSignFor,               3,  4   },
            {   "submit",               &RPCParser::parseSignSubmit,            1,  3   },
            {   "submit_multisigned",   &RPCParser::parseSubmitMultiSigned,     1,  1   },
            {   "server_info",          &RPCParser::parseServerInfo,            0,  1   },
            {   "server_state",         &RPCParser::parseServerInfo,            0,  1   },
            {   "crawl_shards",         &RPCParser::parseAsIs,                  0,  2   },
            {   "stop",                 &RPCParser::parseAsIs,                  0,  0   },
            {   "transaction_entry",    &RPCParser::parseTransactionEntry,      2,  2   },
            {   "tx",                   &RPCParser::parseTx,                    1,  2   },
            {   "tx_account",           &RPCParser::parseTxAccount,             1,  7   },
            {   "tx_history",           &RPCParser::parseTxHistory,             1,  1   },
            {   "unl_list",             &RPCParser::parseAsIs,                  0,  0   },
            {   "validation_create",    &RPCParser::parseValidationCreate,      0,  1   },
            {   "version",              &RPCParser::parseAsIs,                  0,  0   },
            {   "wallet_propose",       &RPCParser::parseWalletPropose,         0,  1   },
            {   "internal",             &RPCParser::parseInternal,              1, -1   },

            {   "path_find",            &RPCParser::parseEvented,              -1, -1   },
            {   "subscribe",            &RPCParser::parseEvented,              -1, -1   },
            {   "unsubscribe",          &RPCParser::parseEvented,              -1, -1   },
        };

        auto const count = jvParams.size ();

        for (auto const& command : commands)
        {
            if (strMethod == command.name)
            {
                if ((command.minParams >= 0 && count < command.minParams) ||
                    (command.maxParams >= 0 && count > command.maxParams))
                {
                    JLOG (j_.debug()) <<
                        "Wrong number of parameters for " << command.name <<
                        " minimum=" << command.minParams <<
                        " maximum=" << command.maxParams <<
                        " actual=" << count;

                    return rpcError (rpcBAD_SYNTAX);
                }

                return (this->* (command.parse)) (jvParams);
            }
        }

        if (!allowAnyCommand)
            return rpcError (rpcUNKNOWN_COMMAND);

        return parseAsIs (jvParams);
    }
};



std::string JSONRPCRequest (std::string const& strMethod, Json::Value const& params, Json::Value const& id)
{
    Json::Value request;
    request[jss::method] = strMethod;
    request[jss::params] = params;
    request[jss::id] = id;
    return to_string (request) + "\n";
}

namespace
{
    class RequestNotParseable : public std::runtime_error
    {
        using std::runtime_error::runtime_error; 
    };
};

struct RPCCallImp
{
    explicit RPCCallImp() = default;

    static void callRPCHandler (Json::Value* jvOutput, Json::Value const& jvInput)
    {
        (*jvOutput) = jvInput;
    }

    static bool onResponse (
        std::function<void (Json::Value const& jvInput)> callbackFuncP,
            const boost::system::error_code& ecResult, int iStatus,
                std::string const& strData, beast::Journal j)
    {
        if (callbackFuncP)
        {

            if (iStatus == 401)
                Throw<std::runtime_error> (
                    "incorrect rpcuser or rpcpassword (authorization failed)");
            else if ((iStatus >= 400) && (iStatus != 400) && (iStatus != 404) && (iStatus != 500)) 
                Throw<std::runtime_error> (
                    std::string ("server returned HTTP error ") +
                        std::to_string (iStatus));
            else if (strData.empty ())
                Throw<std::runtime_error> ("no response from server");

            JLOG (j.debug()) << "RPC reply: " << strData << std::endl;
            if (strData.find("Unable to parse request") == 0)
                Throw<RequestNotParseable> (strData);
            Json::Reader    reader;
            Json::Value     jvReply;
            if (!reader.parse (strData, jvReply))
                Throw<std::runtime_error> ("couldn't parse reply from server");

            if (!jvReply)
                Throw<std::runtime_error> ("expected reply to have result, error and id properties");

            Json::Value     jvResult (Json::objectValue);

            jvResult["result"] = jvReply;

            (callbackFuncP) (jvResult);
        }

        return false;
    }

    static void onRequest (
        std::string const& strMethod,
        Json::Value const& jvParams,
        std::unordered_map<std::string, std::string> const& headers,
        std::string const& strPath,
        boost::asio::streambuf& sb,
        std::string const& strHost,
        beast::Journal j)
    {
        JLOG (j.debug()) << "requestRPC: strPath='" << strPath << "'";

        std::ostream    osRequest (&sb);
        osRequest <<
                  createHTTPPost (
                      strHost,
                      strPath,
                      JSONRPCRequest (strMethod, jvParams, Json::Value (1)),
                      headers);
    }
};


static Json::Value
rpcCmdLineToJson (std::vector<std::string> const& args,
    Json::Value& retParams, beast::Journal j)
{
    Json::Value jvRequest (Json::objectValue);

    RPCParser   rpParser (j);
    Json::Value jvRpcParams (Json::arrayValue);

    for (int i = 1; i != args.size (); i++)
        jvRpcParams.append (args[i]);

    retParams = Json::Value (Json::objectValue);

    retParams[jss::method] = args[0];
    retParams[jss::params] = jvRpcParams;

    jvRequest   = rpParser.parseCommand (args[0], jvRpcParams, true);

    JLOG (j.trace()) << "RPC Request: " << jvRequest << std::endl;
    return jvRequest;
}

Json::Value
cmdLineToJSONRPC (std::vector<std::string> const& args, beast::Journal j)
{
    Json::Value jv = Json::Value (Json::objectValue);
    auto const paramsObj = rpcCmdLineToJson (args, jv, j);

    jv.clear();

    jv[jss::method] = paramsObj.isMember (jss::method) ?
        paramsObj[jss::method].asString() : args[0];

    if (paramsObj.begin() != paramsObj.end())
    {
        auto& paramsArray = Json::setArray (jv, jss::params);
        paramsArray.append (paramsObj);
    }
    if (paramsObj.isMember(jss::jsonrpc))
        jv[jss::jsonrpc] = paramsObj[jss::jsonrpc];
    if (paramsObj.isMember(jss::ripplerpc))
        jv[jss::ripplerpc] = paramsObj[jss::ripplerpc];
    if (paramsObj.isMember(jss::id))
        jv[jss::id] = paramsObj[jss::id];
    return jv;
}


std::pair<int, Json::Value>
rpcClient(std::vector<std::string> const& args,
    Config const& config, Logs& logs,
    std::unordered_map<std::string, std::string> const& headers)
{
    static_assert(rpcBAD_SYNTAX == 1 && rpcSUCCESS == 0,
        "Expect specific rpc enum values.");
    if (args.empty ())
        return { rpcBAD_SYNTAX, {} }; 

    int         nRet = rpcSUCCESS;
    Json::Value jvOutput;
    Json::Value jvRequest (Json::objectValue);

    try
    {
        Json::Value jvRpc   = Json::Value (Json::objectValue);
        jvRequest = rpcCmdLineToJson (args, jvRpc, logs.journal ("RPCParser"));

        if (jvRequest.isMember (jss::error))
        {
            jvOutput            = jvRequest;
            jvOutput["rpc"]     = jvRpc;
        }
        else
        {
            ServerHandler::Setup setup;
            try
            {
                setup = setup_ServerHandler(
                    config,
                    beast::logstream { logs.journal ("HTTPClient").warn() });
            }
            catch (std::exception const&)
            {
            }

            if (config.rpc_ip)
            {
                setup.client.ip = config.rpc_ip->address().to_string();
                setup.client.port = config.rpc_ip->port();
            }

            Json::Value jvParams (Json::arrayValue);

            if (!setup.client.admin_user.empty ())
                jvRequest["admin_user"] = setup.client.admin_user;

            if (!setup.client.admin_password.empty ())
                jvRequest["admin_password"] = setup.client.admin_password;

            if (jvRequest.isObject())
                jvParams.append (jvRequest);
            else if (jvRequest.isArray())
            {
                for (Json::UInt i = 0; i < jvRequest.size(); ++i)
                    jvParams.append(jvRequest[i]);
            }

            {
                boost::asio::io_service isService;
                RPCCall::fromNetwork (
                    isService,
                    setup.client.ip,
                    setup.client.port,
                    setup.client.user,
                    setup.client.password,
                    "",
                    jvRequest.isMember (jss::method)           
                        ? jvRequest[jss::method].asString ()
                        : jvRequest.isArray()
                           ? "batch"
                           : args[0],
                    jvParams,                               
                    setup.client.secure != 0,                
                    config.quiet(),
                    logs,
                    std::bind (RPCCallImp::callRPCHandler, &jvOutput,
                               std::placeholders::_1),
                    headers);
                isService.run(); 
            }
            if (jvOutput.isMember ("result"))
            {
                jvOutput    = jvOutput["result"];

            }
            else
            {
                Json::Value jvRpcError  = jvOutput;

                jvOutput            = rpcError (rpcJSON_RPC);
                jvOutput["result"]  = jvRpcError;
            }

            if (jvOutput.isMember (jss::error))
            {
                jvOutput["rpc"]             = jvRpc;            
                jvOutput["request_sent"]    = jvRequest;        
            }
        }

        if (jvOutput.isMember (jss::error))
        {
            jvOutput[jss::status]  = "error";
            if (jvOutput.isMember(jss::error_code))
                nRet = std::stoi(jvOutput[jss::error_code].asString());
            else if (jvOutput[jss::error].isMember(jss::error_code))
                nRet = std::stoi(jvOutput[jss::error][jss::error_code].asString());
            else
                nRet = rpcBAD_SYNTAX;
        }

    }
    catch (RequestNotParseable& e)
    {
        jvOutput                = rpcError(rpcINVALID_PARAMS);
        jvOutput["error_what"]  = e.what();
        nRet                    = rpcINVALID_PARAMS;
    }
    catch (std::exception& e)
    {
        jvOutput                = rpcError (rpcINTERNAL);
        jvOutput["error_what"]  = e.what ();
        nRet                    = rpcINTERNAL;
    }

    return { nRet, std::move(jvOutput) };
}


namespace RPCCall {

int fromCommandLine (
    Config const& config,
    const std::vector<std::string>& vCmd,
    Logs& logs)
{
    auto const result = rpcClient(vCmd, config, logs);

    if (result.first != rpcBAD_SYNTAX)
        std::cout << result.second.toStyledString ();

    return result.first;
}


void fromNetwork (
    boost::asio::io_service& io_service,
    std::string const& strIp, const std::uint16_t iPort,
    std::string const& strUsername, std::string const& strPassword,
    std::string const& strPath, std::string const& strMethod,
    Json::Value const& jvParams, const bool bSSL, const bool quiet,
    Logs& logs,
    std::function<void (Json::Value const& jvInput)> callbackFuncP,
    std::unordered_map<std::string, std::string> headers)
{
    auto j = logs.journal ("HTTPClient");

    if (!quiet)
    {
        JLOG(j.info()) << (bSSL ? "Securely connecting to " : "Connecting to ") <<
            strIp << ":" << iPort << std::endl;
    }

    headers["Authorization"] = std::string("Basic ") + base64_encode(
        strUsername + ":" + strPassword);


    constexpr auto RPC_REPLY_MAX_BYTES = megabytes(256);

    using namespace std::chrono_literals;
    auto constexpr RPC_NOTIFY = 10min;

    HTTPClient::request (
        bSSL,
        io_service,
        strIp,
        iPort,
        std::bind (
            &RPCCallImp::onRequest,
            strMethod,
            jvParams,
            headers,
            strPath, std::placeholders::_1, std::placeholders::_2, j),
        RPC_REPLY_MAX_BYTES,
        RPC_NOTIFY,
        std::bind (&RPCCallImp::onResponse, callbackFuncP,
                   std::placeholders::_1, std::placeholders::_2,
                   std::placeholders::_3, j),
        j);
}

} 

} 
























