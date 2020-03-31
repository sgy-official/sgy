

#ifndef RIPPLE_PEERFINDER_CHECKER_H_INCLUDED
#define RIPPLE_PEERFINDER_CHECKER_H_INCLUDED

#include <ripple/beast/net/IPAddressConversion.h>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

namespace ripple {
namespace PeerFinder {


    template <class Protocol = boost::asio::ip::tcp>
class Checker
{
private:
    using error_code = boost::system::error_code;

    struct basic_async_op : boost::intrusive::list_base_hook <
        boost::intrusive::link_mode <boost::intrusive::normal_link>>
    {
        virtual
        ~basic_async_op() = default;

        virtual
        void
        stop() = 0;

        virtual
        void
        operator() (error_code const& ec) = 0;
    };

    template <class Handler>
    struct async_op : basic_async_op
    {
        using socket_type = typename Protocol::socket;
        using endpoint_type = typename Protocol::endpoint;

        Checker& checker_;
        socket_type socket_;
        Handler handler_;

        async_op (Checker& owner, boost::asio::io_service& io_service,
            Handler&& handler);

        ~async_op();

        void
        stop() override;

        void
        operator() (error_code const& ec) override;
    };


    using list_type = typename boost::intrusive::make_list <basic_async_op,
        boost::intrusive::constant_time_size <true>>::type;

    std::mutex mutex_;
    std::condition_variable cond_;
    boost::asio::io_service& io_service_;
    list_type list_;
    bool stop_ = false;

public:
    explicit
    Checker (boost::asio::io_service& io_service);

    
    ~Checker();

    
    void
    stop();

    
    void
    wait();

    
    template <class Handler>
    void
    async_connect (beast::IP::Endpoint const& endpoint, Handler&& handler);

private:
    void
    remove (basic_async_op& op);
};


template <class Protocol>
template <class Handler>
Checker<Protocol>::async_op<Handler>::async_op (Checker& owner,
        boost::asio::io_service& io_service, Handler&& handler)
    : checker_ (owner)
    , socket_ (io_service)
    , handler_ (std::forward<Handler>(handler))
{
}

template <class Protocol>
template <class Handler>
Checker<Protocol>::async_op<Handler>::~async_op()
{
    checker_.remove (*this);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_op<Handler>::stop()
{
    error_code ec;
    socket_.cancel(ec);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_op<Handler>::operator() (
    error_code const& ec)
{
    handler_(ec);
}


template <class Protocol>
Checker<Protocol>::Checker (boost::asio::io_service& io_service)
    : io_service_(io_service)
{
}

template <class Protocol>
Checker<Protocol>::~Checker()
{
    wait();
}

template <class Protocol>
void
Checker<Protocol>::stop()
{
    std::lock_guard<std::mutex> lock (mutex_);
    if (! stop_)
    {
        stop_ = true;
        for (auto& c : list_)
            c.stop();
    }
}

template <class Protocol>
void
Checker<Protocol>::wait()
{
    std::unique_lock<std::mutex> lock (mutex_);
    while (! list_.empty())
        cond_.wait (lock);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_connect (
    beast::IP::Endpoint const& endpoint, Handler&& handler)
{
    auto const op = std::make_shared<async_op<Handler>> (
        *this, io_service_, std::forward<Handler>(handler));
    {
        std::lock_guard<std::mutex> lock (mutex_);
        list_.push_back (*op);
    }
    op->socket_.async_connect (beast::IPAddressConversion::to_asio_endpoint (
        endpoint), std::bind (&basic_async_op::operator(), op,
            std::placeholders::_1));
}

template <class Protocol>
void
Checker<Protocol>::remove (basic_async_op& op)
{
    std::lock_guard <std::mutex> lock (mutex_);
    list_.erase (list_.iterator_to (op));
    if (list_.size() == 0)
        cond_.notify_all();
}

}
}

#endif








