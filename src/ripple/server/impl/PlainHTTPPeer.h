

#ifndef RIPPLE_SERVER_PLAINHTTPPEER_H_INCLUDED
#define RIPPLE_SERVER_PLAINHTTPPEER_H_INCLUDED

#include <ripple/beast/rfc2616.h>
#include <ripple/server/impl/BaseHTTPPeer.h>
#include <ripple/server/impl/PlainWSPeer.h>
#include <memory>

namespace ripple {

template<class Handler>
class PlainHTTPPeer
    : public BaseHTTPPeer<Handler, PlainHTTPPeer<Handler>>
    , public std::enable_shared_from_this<PlainHTTPPeer<Handler>>
{
private:
    friend class BaseHTTPPeer<Handler, PlainHTTPPeer>;
    using waitable_timer = typename BaseHTTPPeer<Handler, PlainHTTPPeer>::waitable_timer;
    using socket_type = boost::asio::ip::tcp::socket;
    using endpoint_type = boost::asio::ip::tcp::endpoint;

    socket_type stream_;

public:
    template <class ConstBufferSequence>
    PlainHTTPPeer(
        Port const& port,
        Handler& handler,
        boost::asio::io_context& ioc,
        beast::Journal journal,
        endpoint_type remote_address,
        ConstBufferSequence const& buffers,
        socket_type&& socket);

    void run();

    std::shared_ptr<WSSession>
    websocketUpgrade() override;

private:
    void
    do_request() override;

    void
    do_close() override;
};


template <class Handler>
template <class ConstBufferSequence>
PlainHTTPPeer<Handler>::PlainHTTPPeer(
    Port const& port,
    Handler& handler,
    boost::asio::io_context& ioc,
    beast::Journal journal,
    endpoint_type remote_endpoint,
    ConstBufferSequence const& buffers,
    socket_type&& socket)
    : BaseHTTPPeer<Handler, PlainHTTPPeer>(
          port,
          handler,
          ioc.get_executor(),
          waitable_timer{ioc},
          journal,
          remote_endpoint,
          buffers)
    , stream_(std::move(socket))
{
    if(remote_endpoint.address().is_loopback())
        stream_.set_option(boost::asio::ip::tcp::no_delay{true});
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
run()
{
    if (! this->handler_.onAccept(this->session(), this->remote_address_))
    {
        boost::asio::spawn(this->strand_,
            std::bind (&PlainHTTPPeer::do_close,
                this->shared_from_this()));
        return;
    }

    if (! stream_.is_open())
        return;

    boost::asio::spawn(this->strand_, std::bind(&PlainHTTPPeer::do_read,
        this->shared_from_this(), std::placeholders::_1));
}

template<class Handler>
std::shared_ptr<WSSession>
PlainHTTPPeer<Handler>::
websocketUpgrade()
{
    auto ws = this->ios().template emplace<PlainWSPeer<Handler>>(
        this->port_, this->handler_, this->remote_address_,
            std::move(this->message_), std::move(stream_),
                this->journal_);
    return ws;
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
do_request()
{
    ++this->request_count_;
    auto const what = this->handler_.onHandoff(this->session(),
        std::move(this->message_), this->remote_address_);
    if (what.moved)
        return;
    boost::system::error_code ec;
    if (what.response)
    {
        if (! what.keep_alive)
            stream_.shutdown(socket_type::shutdown_receive, ec);
        if (ec)
            return this->fail(ec, "request");
        return this->write(what.response, what.keep_alive);
    }

    if (! beast::rfc2616::is_keep_alive(this->message_))
        stream_.shutdown(socket_type::shutdown_receive, ec);
    if (ec)
        return this->fail(ec, "request");
    this->handler_.onRequest(this->session());
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
do_close()
{
    boost::system::error_code ec;
    stream_.shutdown(socket_type::shutdown_send, ec);
}

} 

#endif








