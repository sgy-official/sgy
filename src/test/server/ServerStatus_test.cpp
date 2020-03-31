

#include <ripple/rpc/ServerHandler.h>
#include <ripple/json/json_reader.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <test/jtx.h>
#include <test/jtx/envconfig.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/JSONRPCClient.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/basics/base64.h>
#include <boost/beast/http.hpp>
#include <beast/test/yield_to.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <algorithm>
#include <array>
#include <random>
#include <regex>

namespace ripple {
namespace test {

class ServerStatus_test :
    public beast::unit_test::suite, public beast::test::enable_yield_to
{
    class myFields : public boost::beast::http::fields {};

    auto makeConfig(
        std::string const& proto,
        bool admin = true,
        bool credentials = false)
    {
        auto const section_name =
            boost::starts_with(proto, "h") ?  "port_rpc" : "port_ws";
        auto p = jtx::envconfig();

        p->overwrite(section_name, "protocol", proto);
        if(! admin)
            p->overwrite(section_name, "admin", "");

        if(credentials)
        {
            (*p)[section_name].set("admin_password", "p");
            (*p)[section_name].set("admin_user", "u");
        }

        p->overwrite(
            boost::starts_with(proto, "h") ?  "port_ws" : "port_rpc",
            "protocol",
            boost::starts_with(proto, "h") ?  "ws" : "http");

        if(proto == "https")
        {
            (*p)["server"].append("port_alt");
            (*p)["port_alt"].set("ip", getEnvLocalhostAddr());
            (*p)["port_alt"].set("port", "8099");
            (*p)["port_alt"].set("protocol", "http");
            (*p)["port_alt"].set("admin", getEnvLocalhostAddr());
        }

        return p;
    }

    auto makeWSUpgrade(
        std::string const& host,
        uint16_t port)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        request<string_body> req;

        req.target("/");
        req.version(11);
        req.insert("Host", host + ":" + std::to_string(port));
        req.insert("User-Agent", "test");
        req.method(boost::beast::http::verb::get);
        req.insert("Upgrade", "websocket");
        {
            std::random_device rd;
            std::mt19937 e{rd()};
            std::uniform_int_distribution<> d(0, 255);
            std::array<std::uint8_t, 16> key;
            for(auto& v : key)
                v = d(e);
            req.insert("Sec-WebSocket-Key", base64_encode(key.data(), key.size()));
        };
        req.insert("Sec-WebSocket-Version", "13");
        req.insert(boost::beast::http::field::connection, "upgrade");
        return req;
    }

    auto makeHTTPRequest(
        std::string const& host,
        uint16_t port,
        std::string const& body,
        myFields const& fields)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        request<string_body> req;

        req.target("/");
        req.version(11);
        for(auto const& f : fields)
            req.insert(f.name(), f.value());
        req.insert("Host", host + ":" + std::to_string(port));
        req.insert("User-Agent", "test");
        if(body.empty())
        {
            req.method(boost::beast::http::verb::get);
        }
        else
        {
            req.method(boost::beast::http::verb::post);
            req.insert("Content-Type", "application/json; charset=UTF-8");
            req.body() = body;
        }
        req.prepare_payload();

        return req;
    }

    void
    doRequest(
        boost::asio::yield_context& yield,
        boost::beast::http::request<boost::beast::http::string_body>&& req,
        std::string const& host,
        uint16_t port,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};
        boost::beast::multi_buffer sb;

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{host, std::to_string(port)}, yield[ec]);
        if(ec)
            return;

        if(secure)
        {
            ssl::context ctx{ssl::context::sslv23};
            ctx.set_verify_mode(ssl::verify_none);
            ssl::stream<ip::tcp::socket> ss{ios, ctx};
            async_connect(ss.next_layer(), it, yield[ec]);
            if(ec)
                return;
            ss.async_handshake(ssl::stream_base::client, yield[ec]);
            if(ec)
                return;
            boost::beast::http::async_write(ss, req, yield[ec]);
            if(ec)
                return;
            async_read(ss, sb, resp, yield[ec]);
            if(ec)
                return;
        }
        else
        {
            ip::tcp::socket sock{ios};
            async_connect(sock, it, yield[ec]);
            if(ec)
                return;
            boost::beast::http::async_write(sock, req, yield[ec]);
            if(ec)
                return;
            async_read(sock, sb, resp, yield[ec]);
            if(ec)
                return;
        }

        return;
    }

    void
    doWSRequest(
        test::jtx::Env& env,
        boost::asio::yield_context& yield,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec)
    {
        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto ip = env.app().config()["port_ws"].
            get<std::string>("ip");
        doRequest(
            yield,
            makeWSUpgrade(*ip, *port),
            *ip,
            *port,
            secure,
            resp,
            ec);
        return;
    }

    void
    doHTTPRequest(
        test::jtx::Env& env,
        boost::asio::yield_context& yield,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec,
        std::string const& body = "",
        myFields const& fields = {})
    {
        auto const port = env.app().config()["port_rpc"].
            get<std::uint16_t>("port");
        auto const ip = env.app().config()["port_rpc"].
            get<std::string>("ip");
        doRequest(
            yield,
            makeHTTPRequest(*ip, *port, body, fields),
            *ip,
            *port,
            secure,
            resp,
            ec);
        return;
    }

    auto makeAdminRequest(
        jtx::Env & env,
        std::string const& proto,
        std::string const& user,
        std::string const& password,
        bool subobject = false)
    {
        Json::Value jrr;

        Json::Value jp = Json::objectValue;
        if(! user.empty())
        {
            jp["admin_user"] = user;
            if(subobject)
            {
                Json::Value jpi = Json::objectValue;
                jpi["admin_password"] = password;
                jp["admin_password"] = jpi;
            }
            else
            {
                jp["admin_password"] = password;
            }
        }

        if(boost::starts_with(proto, "h"))
        {
            auto jrc = makeJSONRPCClient(env.app().config());
            jrr = jrc->invoke("ledger_accept", jp);
        }
        else
        {
            auto wsc = makeWSClient(env.app().config(), proto == "ws2");
            jrr = wsc->invoke("ledger_accept", jp);
        }

        return jrr;
    }


    void
    testAdminRequest(std::string const& proto, bool admin, bool credentials)
    {
        testcase << "Admin request over " << proto <<
            ", config " << (admin ? "enabled" : "disabled") <<
            ", credentials " << (credentials ? "" : "not ") << "set";
        using namespace jtx;
        Env env {*this, makeConfig(proto, admin, credentials)};

        Json::Value jrr;
        auto const proto_ws = boost::starts_with(proto, "w");


        if(admin && credentials)
        {
            auto const user = env.app().config()
                [proto_ws ? "port_ws" : "port_rpc"].
                get<std::string>("admin_user");

            auto const password = env.app().config()
                [proto_ws ? "port_ws" : "port_rpc"].
                get<std::string>("admin_password");

            jrr = makeAdminRequest(env, proto, *user, *password + "_")
                [jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

            jrr = makeAdminRequest(env, proto, *user, *password, true)[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

            jrr = makeAdminRequest(env, proto, *user + "_", *password)[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

            jrr = makeAdminRequest(env, proto, *user, *password)[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");
        }
        else if(admin)
        {
            jrr = makeAdminRequest(env, proto, "u", "p")[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");

            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");
        }
        else
        {
            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");
        }
    }

    void
    testWSClientToHttpServer(boost::asio::yield_context& yield)
    {
        testcase("WS client to http server fails");
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_ws").set("protocol", "http,https");
                return cfg;
            })};

        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doWSRequest(env, yield, false, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::unauthorized);
        }

        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doWSRequest(env, yield, true, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::unauthorized);
        }
    }

    void
    testStatusRequest(boost::asio::yield_context& yield)
    {
        testcase("Status request");
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_rpc").set("protocol", "ws2,wss2");
                cfg->section("port_ws").set("protocol", "http");
                return cfg;
            })};

        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, false, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        }

        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, true, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        }
    }

    void
    testTruncatedWSUpgrade(boost::asio::yield_context& yield)
    {
        testcase("Partial WS upgrade request");
        using namespace jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_ws").set("protocol", "ws2");
                return cfg;
            })};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip");

        boost::system::error_code ec;
        response<string_body> resp;
        auto req = makeWSUpgrade(*ip, *port);

        auto req_string = boost::lexical_cast<std::string>(req);
        req_string.erase(req_string.find_last_of("13"), std::string::npos);

        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};
        boost::beast::multi_buffer sb;

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{*ip, std::to_string(*port)}, yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;

        ip::tcp::socket sock{ios};
        async_connect(sock, it, yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        async_write(sock, boost::asio::buffer(req_string), yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        async_read(sock, sb, resp, yield[ec]);
        BEAST_EXPECT(ec);
    }

    void
    testCantConnect(
        std::string const& client_protocol,
        std::string const& server_protocol,
        boost::asio::yield_context& yield)
    {
        testcase << "Connect fails: " << client_protocol << " client to " <<
            server_protocol << " server";
        using namespace jtx;
        Env env {*this, makeConfig(server_protocol)};

        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        if(boost::starts_with(client_protocol, "h"))
        {
                doHTTPRequest(
                    env,
                    yield,
                    client_protocol == "https",
                    resp,
                    ec);
                BEAST_EXPECT(ec);
        }
        else
        {
                doWSRequest(
                    env,
                    yield,
                    client_protocol == "wss" || client_protocol == "wss2",
                    resp,
                    ec);
                BEAST_EXPECT(ec);
        }
    }

    void
    testAuth(bool secure, boost::asio::yield_context& yield)
    {
        testcase << "Server with authorization, " <<
            (secure ? "secure" : "non-secure");

        using namespace test::jtx;
        Env env {*this, envconfig([secure](std::unique_ptr<Config> cfg) {
            (*cfg)["port_rpc"].set("user","me");
            (*cfg)["port_rpc"].set("password","secret");
            (*cfg)["port_rpc"].set("protocol", secure ? "https" : "http");
            if (secure)
                (*cfg)["port_ws"].set("protocol","http,ws");
            return cfg;
        })};

        Json::Value jr;
        jr[jss::method] = "server_info";
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr));
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        myFields auth;
        auth.insert("Authorization", "");
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auth.set("Authorization", "Basic NOT-VALID");
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auth.set("Authorization", "Basic " + base64_encode("me:badpass"));
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auto const user = env.app().config().section("port_rpc").
            get<std::string>("user").value();
        auto const pass = env.app().config().section("port_rpc").
            get<std::string>("password").value();

        auth.set("Authorization", "Basic " + user + ":" + pass);
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auth.set("Authorization", "Basic " +
            base64_encode(user + ":" + pass));
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(! resp.body().empty());
    }

    void
    testLimit(boost::asio::yield_context& yield, int limit)
    {
        testcase << "Server with connection limit of " << limit;

        using namespace test::jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, envconfig([&](std::unique_ptr<Config> cfg) {
            (*cfg)["port_rpc"].set("limit", to_string(limit));
            return cfg;
        })};


        auto const port = env.app().config()["port_rpc"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_rpc"].
            get<std::string>("ip").value();

        boost::system::error_code ec;
        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};

        Json::Value jr;
        jr[jss::method] = "server_info";

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{ip, to_string(port)}, yield[ec]);
        BEAST_EXPECT(! ec);

        std::vector<std::pair<ip::tcp::socket, boost::beast::multi_buffer>> clients;
        int connectionCount {1}; 


        int testTo = (limit == 0) ? 50 : limit + 1;
        while (connectionCount < testTo)
        {
            clients.emplace_back(
                std::make_pair(ip::tcp::socket {ios}, boost::beast::multi_buffer{}));
            async_connect(clients.back().first, it, yield[ec]);
            BEAST_EXPECT(! ec);
            auto req = makeHTTPRequest(ip, port, to_string(jr), {});
            async_write(
                clients.back().first,
                req,
                yield[ec]);
            BEAST_EXPECT(! ec);
            ++connectionCount;
        }

        int readCount = 0;
        for (auto& c : clients)
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            async_read(c.first, c.second, resp, yield[ec]);
            ++readCount;
            BEAST_EXPECT((limit == 0 || readCount < limit-1) ? (! ec) : bool(ec));
        }
    }

    void
    testWSHandoff(boost::asio::yield_context& yield)
    {
        testcase ("Connection with WS handoff");

        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg) {
            (*cfg)["port_ws"].set("protocol","wss");
            return cfg;
        })};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doRequest(
            yield, makeWSUpgrade(ip, port), ip, port, true, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::switching_protocols);
        BEAST_EXPECT(resp.find("Upgrade") != resp.end() &&
               resp["Upgrade"] == "websocket");
        BEAST_EXPECT(resp.find("Connection") != resp.end() &&
               resp["Connection"] == "upgrade");
    }

    void
    testNoRPC(boost::asio::yield_context& yield)
    {
        testcase ("Connection to port with no RPC enabled");

        using namespace test::jtx;
        Env env {*this};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doRequest(yield,
            makeHTTPRequest(ip, port, "foo", {}), ip, port, false, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);
        BEAST_EXPECT(resp.body() == "Forbidden\r\n");
    }

    void
    testWSRequests(boost::asio::yield_context& yield)
    {
        testcase ("WS client sends assorted input");

        using namespace test::jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::system::error_code ec;

        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{ip, to_string(port)}, yield[ec]);
        if(! BEAST_EXPECT(! ec))
            return;

        ip::tcp::socket sock{ios};
        async_connect(sock, it, yield[ec]);
        if(! BEAST_EXPECT(! ec))
            return;

        boost::beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{sock};
        ws.handshake(ip + ":" + to_string(port), "/");

        auto sendAndParse = [&](std::string const& req) -> Json::Value
        {
            ws.async_write_some(true, buffer(req), yield[ec]);
            if(! BEAST_EXPECT(! ec))
                return Json::objectValue;

            boost::beast::multi_buffer sb;
            ws.async_read(sb, yield[ec]);
            if(! BEAST_EXPECT(! ec))
                return Json::objectValue;

            Json::Value resp;
            Json::Reader jr;
            if(! BEAST_EXPECT(jr.parse(
                boost::lexical_cast<std::string>(
                    boost::beast::buffers(sb.data())), resp)))
                return Json::objectValue;
            sb.consume(sb.size());
            return resp;
        };

        { 
            auto resp = sendAndParse("NOT JSON");
            BEAST_EXPECT(resp.isMember(jss::error) &&
                resp[jss::error] == "jsonInvalid");
            BEAST_EXPECT(! resp.isMember(jss::status));
        }

        { 
            Json::Value jv;
            jv[jss::command] = "foo";
            jv[jss::method] = "bar";
            auto resp = sendAndParse(to_string(jv));
            BEAST_EXPECT(resp.isMember(jss::error) &&
                resp[jss::error] == "missingCommand");
            BEAST_EXPECT(resp.isMember(jss::status) &&
                resp[jss::status] == "error");
        }

        { 
            Json::Value jv;
            jv[jss::command] = "ping";
            auto resp = sendAndParse(to_string(jv));
            BEAST_EXPECT(resp.isMember(jss::status) &&
                resp[jss::status] == "success");
            BEAST_EXPECT(resp.isMember(jss::result) &&
                resp[jss::result].isMember(jss::role) &&
                resp[jss::result][jss::role] == "admin");
        }
    }

    void
    testAmendmentBlock(boost::asio::yield_context& yield)
    {
        testcase("Status request over WS and RPC with/without Amendment Block");
        using namespace jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, validator( envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_rpc").set("protocol", "http");
                return cfg;
            }), "")};

        env.close();

        env.app().getLedgerMaster().tryAdvance();

        auto si = env.rpc("server_info") [jss::result];
        BEAST_EXPECT(! si[jss::info].isMember(jss::amendment_blocked));
        BEAST_EXPECT(
            env.app().getOPs().getConsensusInfo()["validating"] == true);

        auto const port_ws = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto const ip_ws = env.app().config()["port_ws"].
            get<std::string>("ip");


        boost::system::error_code ec;
        response<string_body> resp;

        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(
            resp.body().find("connectivity is working.") != std::string::npos);

        env.app().getOPs().setAmendmentBlocked ();
        env.app().getOPs().beginConsensus(env.closed()->info().hash);

        BEAST_EXPECT(
            env.app().getOPs().getConsensusInfo()["validating"] == false);

        si = env.rpc("server_info") [jss::result];
        BEAST_EXPECT(
            si[jss::info].isMember(jss::amendment_blocked) &&
            si[jss::info][jss::amendment_blocked] == true);

        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(
            resp.body().find("connectivity is working.") != std::string::npos);

        env.app().config().ELB_SUPPORT = true;

        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::internal_server_error);
        BEAST_EXPECT(
            resp.body().find("cannot accept clients:") != std::string::npos);
        BEAST_EXPECT(
            resp.body().find("Server version too old") != std::string::npos);
    }

    void
    testRPCRequests(boost::asio::yield_context& yield)
    {
        testcase ("RPC client sends assorted input");

        using namespace test::jtx;
        Env env {*this};

        boost::system::error_code ec;
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, false, resp, ec, "{}");
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv["invalid"] = 1;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Null method\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv(Json::arrayValue);
            jv.append("invalid");
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv(Json::arrayValue);
            Json::Value j;
            j["invalid"] = 1;
            jv.append(j);
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv[jss::method] = "batch";
            jv[jss::params] = 2;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Malformed batch request\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv[jss::method] = "batch";
            jv[jss::params] = Json::objectValue;
            jv[jss::params]["invalid"] = 3;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Malformed batch request\r\n");
        }

        Json::Value jv;
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = Json::nullValue;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Null method\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = 1;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "method is not string\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = "";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "method is empty\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = "some_method";
            jv[jss::params] = "params";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "params unparseable\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::params] = Json::arrayValue;
            jv[jss::params][0u] = "not an object";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "params unparseable\r\n");
        }
    }

    void
    testStatusNotOkay(boost::asio::yield_context& yield)
    {
        testcase ("Server status not okay");

        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->ELB_SUPPORT = true;
            return cfg;
        })};

        env.app().getFeeTrack().raiseLocalFee();

        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doHTTPRequest(env, yield, false, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::internal_server_error);
        std::regex body {"Server cannot accept clients"};
        BEAST_EXPECT(std::regex_search(resp.body(), body));
    }

public:
    void
    run() override
    {
        for (auto it : {"http", "ws", "ws2"})
        {
            testAdminRequest (it, true, true);
            testAdminRequest (it, true, false);
            testAdminRequest (it, false, false);
        }

        yield_to([&](boost::asio::yield_context& yield)
        {
            testWSClientToHttpServer (yield);
            testStatusRequest (yield);
            testTruncatedWSUpgrade (yield);

            testCantConnect ("ws", "wss", yield);
            testCantConnect ("ws2", "wss2", yield);
            testCantConnect ("http", "https", yield);
            testCantConnect ("wss", "ws", yield);
            testCantConnect ("wss2", "ws2", yield);
            testCantConnect ("https", "http", yield);

            testAmendmentBlock(yield);
            testAuth (false, yield);
            testAuth (true, yield);
            testLimit (yield, 5);
            testLimit (yield, 0);
            testWSHandoff (yield);
            testNoRPC (yield);
            testWSRequests (yield);
            testRPCRequests (yield);
            testStatusNotOkay (yield);
        });

    }
};

BEAST_DEFINE_TESTSUITE(ServerStatus, server, ripple);

} 
} 

























