

#ifndef RIPPLE_PEERFINDER_MAKE_MANAGER_H_INCLUDED
#define RIPPLE_PEERFINDER_MAKE_MANAGER_H_INCLUDED

#include <ripple/peerfinder/PeerfinderManager.h>
#include <boost/asio/io_service.hpp>
#include <memory>

namespace ripple {
namespace PeerFinder {


std::unique_ptr<Manager>
make_Manager (Stoppable& parent, boost::asio::io_service& io_service,
        clock_type& clock, beast::Journal journal, BasicConfig const& config);

}
}

#endif








