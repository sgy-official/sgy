

#ifndef RIPPLE_OVERLAY_MAKE_OVERLAY_H_INCLUDED
#define RIPPLE_OVERLAY_MAKE_OVERLAY_H_INCLUDED

#include <ripple/rpc/ServerHandler.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/basics/Resolver.h>
#include <ripple/core/Stoppable.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ssl/context.hpp>

namespace ripple {

Overlay::Setup
setup_Overlay (BasicConfig const& config);


std::unique_ptr <Overlay>
make_Overlay (
    Application& app,
    Overlay::Setup const& setup,
    Stoppable& parent,
    ServerHandler& serverHandler,
    Resource::Manager& resourceManager,
    Resolver& resolver,
    boost::asio::io_service& io_service,
    BasicConfig const& config);

} 

#endif








