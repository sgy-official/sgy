

#ifndef RIPPLE_APP_BASICAPP_H_INCLUDED
#define RIPPLE_APP_BASICAPP_H_INCLUDED

#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>
#include <thread>
#include <vector>

class BasicApp
{
private:
    boost::optional<boost::asio::io_service::work> work_;
    std::vector<std::thread> threads_;
    boost::asio::io_service io_service_;

protected:
    BasicApp(std::size_t numberOfThreads);
    ~BasicApp();

public:
    boost::asio::io_service&
    get_io_service()
    {
        return io_service_;
    }
};

#endif








