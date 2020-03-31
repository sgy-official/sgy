
#ifndef BEAST_TEST_SIG_WAIT_HPP
#define BEAST_TEST_SIG_WAIT_HPP

#include <boost/asio.hpp>

namespace beast {
namespace test {

inline
void
sig_wait()
{
    boost::asio::io_service ios;
    boost::asio::signal_set signals(
        ios, SIGINT, SIGTERM);
    signals.async_wait(
        [&](boost::system::error_code const&, int)
        {
        });
    ios.run();
}

} 
} 

#endif






