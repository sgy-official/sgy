

#ifndef RIPPLE_SERVER_SESSION_H_INCLUDED
#define RIPPLE_SERVER_SESSION_H_INCLUDED

#include <ripple/server/Writer.h>
#include <ripple/server/WSSession.h>
#include <boost/beast/http/message.hpp>
#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>
#include <memory>
#include <ostream>
#include <vector>

namespace ripple {


class Session
{
public:
    Session() = default;
    Session (Session const&) = delete;
    Session& operator=(Session const&) = delete;
    virtual ~Session () = default;

    
    void* tag = nullptr;

    
    virtual
    beast::Journal
    journal() = 0;

    
    virtual
    Port const&
    port() = 0;

    
    virtual
    beast::IP::Endpoint
    remoteAddress() = 0;

    
    virtual
    http_request_type&
    request() = 0;

    
    
    void
    write (std::string const& s)
    {
        if (! s.empty())
            write (&s[0],
                std::distance (s.begin(), s.end()));
    }

    template <typename BufferSequence>
    void
    write (BufferSequence const& buffers)
    {
        for (typename BufferSequence::const_iterator iter (buffers.begin());
            iter != buffers.end(); ++iter)
        {
            typename BufferSequence::value_type const& buffer (*iter);
            write (boost::asio::buffer_cast <void const*> (buffer),
                boost::asio::buffer_size (buffer));
        }
    }

    virtual
    void
    write (void const* buffer, std::size_t bytes) = 0;

    virtual
    void
    write (std::shared_ptr <Writer> const& writer,
        bool keep_alive) = 0;

    

    
    virtual
    std::shared_ptr<Session>
    detach() = 0;

    
    virtual
    void
    complete() = 0;

    
    virtual
    void
    close (bool graceful) = 0;

    
    virtual
    std::shared_ptr<WSSession>
    websocketUpgrade() = 0;
};

}  

#endif








