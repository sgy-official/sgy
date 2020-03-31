

#ifndef RIPPLE_SERVER_HANDOFF_H_INCLUDED
#define RIPPLE_SERVER_HANDOFF_H_INCLUDED

#include <ripple/server/Writer.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <memory>

namespace ripple {

using http_request_type =
    boost::beast::http::request<boost::beast::http::dynamic_body>;

using http_response_type =
    boost::beast::http::response<boost::beast::http::dynamic_body>;


struct Handoff
{
    bool moved = false;

    bool keep_alive = false;

    std::shared_ptr<Writer> response;

    bool handled() const
    {
        return moved || response;
    }
};

} 

#endif








