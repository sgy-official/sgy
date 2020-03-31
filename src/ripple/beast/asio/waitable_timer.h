

#ifndef RIPPLE_BEAST_ASIO_WAITABLETIMER_H_INCLUDED
#define RIPPLE_BEAST_ASIO_WAITABLETIMER_H_INCLUDED

#include <boost/asio/ip/tcp.hpp>

namespace beast {

template <class T>
T
create_waitable_timer(boost::asio::ip::tcp::socket& socket)
{
#if BOOST_VERSION >= 107000
    return T{socket.get_executor()};
#else
    return T{socket.get_io_service()};
#endif
}

}  

#endif








