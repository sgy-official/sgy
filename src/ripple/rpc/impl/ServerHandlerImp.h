

#ifndef RIPPLE_RPC_SERVERHANDLERIMP_H_INCLUDED
#define RIPPLE_RPC_SERVERHANDLERIMP_H_INCLUDED

#include <ripple/core/JobQueue.h>
#include <ripple/rpc/impl/WSInfoSub.h>
#include <ripple/server/Server.h>
#include <ripple/server/Session.h>
#include <ripple/server/WSSession.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/app/main/CollectorManager.h>
#include <ripple/json/Output.h>
#include <boost/utility/string_view.hpp>
#include <map>
#include <mutex>
#include <vector>

namespace ripple {

inline
bool operator< (Port const& lhs, Port const& rhs)
{
    return lhs.name < rhs.name;
}

class ServerHandlerImp
    : public Stoppable
{
public:
    struct Setup
    {
        explicit Setup() = default;

        std::vector<Port> ports;

        struct client_t
        {
            explicit client_t() = default;

            bool secure = false;
            std::string ip;
            std::uint16_t port = 0;
            std::string user;
            std::string password;
            std::string admin_user;
            std::string admin_password;
        };

        client_t client;

        struct overlay_t
        {
            explicit overlay_t() = default;

            boost::asio::ip::address ip;
            std::uint16_t port = 0;
        };

        overlay_t overlay;

        void
        makeContexts();
    };

private:

    Application& app_;
    Resource::Manager& m_resourceManager;
    beast::Journal m_journal;
    NetworkOPs& m_networkOPs;
    std::unique_ptr<Server> m_server;
    Setup setup_;
    JobQueue& m_jobQueue;
    beast::insight::Counter rpc_requests_;
    beast::insight::Event rpc_size_;
    beast::insight::Event rpc_time_;
    std::mutex countlock_;
    std::map<std::reference_wrapper<Port const>, int> count_;

public:
    ServerHandlerImp (Application& app, Stoppable& parent,
        boost::asio::io_service& io_service, JobQueue& jobQueue,
            NetworkOPs& networkOPs, Resource::Manager& resourceManager,
                CollectorManager& cm);

    ~ServerHandlerImp();

    using Output = Json::Output;

    void
    setup (Setup const& setup, beast::Journal journal);

    Setup const&
    setup() const
    {
        return setup_;
    }


    void
    onStop() override;


    bool
    onAccept (Session& session,
        boost::asio::ip::tcp::endpoint endpoint);

    Handoff
    onHandoff(
        Session& session,
        std::unique_ptr<beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
        boost::asio::ip::tcp::endpoint const& remote_address);

    Handoff
    onHandoff(
        Session& session,
        http_request_type&& request,
        boost::asio::ip::tcp::endpoint const& remote_address)
    {
        return onHandoff(
            session,
            {},
            std::forward<http_request_type>(request),
            remote_address);
    }

    void
    onRequest (Session& session);

    void
    onWSMessage(std::shared_ptr<WSSession> session,
        std::vector<boost::asio::const_buffer> const& buffers);

    void
    onClose (Session& session,
        boost::system::error_code const&);

    void
    onStopped (Server&);

private:
    Json::Value
    processSession(
        std::shared_ptr<WSSession> const& session,
            std::shared_ptr<JobQueue::Coro> const& coro,
                Json::Value const& jv);

    void
    processSession (std::shared_ptr<Session> const&,
        std::shared_ptr<JobQueue::Coro> coro);

    void
    processRequest (Port const& port, std::string const& request,
        beast::IP::Endpoint const& remoteIPAddress, Output&&,
        std::shared_ptr<JobQueue::Coro> coro,
        boost::string_view forwardedFor, boost::string_view user);

    Handoff
    statusResponse(http_request_type const& request) const;
};

}

#endif








