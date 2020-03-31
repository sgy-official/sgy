

#ifndef RIPPLE_OVERLAY_TMHELLO_H_INCLUDED
#define RIPPLE_OVERLAY_TMHELLO_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <ripple/app/main/Application.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/protocol/BuildInfo.h>

#include <boost/beast/http/message.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>
#include <utility>

namespace ripple {

enum
{
    clockToleranceDeltaSeconds = 20
};


boost::optional<uint256>
makeSharedValue (SSL* ssl, beast::Journal journal);


protocol::TMHello
buildHello (uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote, Application& app);


void
appendHello (boost::beast::http::fields& h, protocol::TMHello const& hello);


boost::optional<protocol::TMHello>
parseHello (bool request, boost::beast::http::fields const& h, beast::Journal journal);


boost::optional<PublicKey>
verifyHello (protocol::TMHello const& h, uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote,
    beast::Journal journal, Application& app);


std::vector<ProtocolVersion>
parse_ProtocolVersions(boost::beast::string_view const& s);

}

#endif








