

#ifndef RIPPLE_RPC_SERVERHANDLER_H_INCLUDED
#define RIPPLE_RPC_SERVERHANDLER_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/core/Config.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Stoppable.h>
#include <ripple/server/Port.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/rpc/impl/ServerHandlerImp.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <memory>
#include <vector>

namespace ripple {

using ServerHandler = ServerHandlerImp;

ServerHandler::Setup
setup_ServerHandler (
    Config const& c,
    std::ostream&& log);

std::unique_ptr <ServerHandler>
make_ServerHandler (Application& app, Stoppable& parent, boost::asio::io_service&,
    JobQueue&, NetworkOPs&, Resource::Manager&, CollectorManager& cm);

} 

#endif








