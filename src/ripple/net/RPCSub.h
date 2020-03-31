

#ifndef RIPPLE_NET_RPCSUB_H_INCLUDED
#define RIPPLE_NET_RPCSUB_H_INCLUDED

#include <ripple/core/JobQueue.h>
#include <ripple/net/InfoSub.h>
#include <ripple/core/Stoppable.h>
#include <boost/asio/io_service.hpp>

namespace ripple {


class RPCSub : public InfoSub
{
public:
    virtual void setUsername (std::string const& strUsername) = 0;
    virtual void setPassword (std::string const& strPassword) = 0;

protected:
    explicit RPCSub (InfoSub::Source& source);
};

std::shared_ptr<RPCSub> make_RPCSub (
    InfoSub::Source& source, boost::asio::io_service& io_service,
    JobQueue& jobQueue, std::string const& strUrl,
    std::string const& strUsername, std::string const& strPassword,
    Logs& logs);

} 

#endif








