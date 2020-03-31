

#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <ripple/protocol/jss.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/Overlay.h>
#include <unordered_map>

namespace ripple {

class Peers_test : public beast::unit_test::suite
{
    void testRequest()
    {
        testcase("Basic request");
        using namespace test::jtx;
        Env env {*this};

        auto peers = env.rpc("peers")[jss::result];
        BEAST_EXPECT(peers.isMember(jss::cluster) &&
            peers[jss::cluster].size() == 0 );
        BEAST_EXPECT(peers.isMember(jss::peers) &&
            peers[jss::peers].isNull());

        std::unordered_map<std::string, std::string> nodes;
        for(auto i =0; i < 3; ++i)
        {

            auto kp = generateKeyPair (KeyType::secp256k1,
                generateSeed("seed" + std::to_string(i)));

            std::string name = "Node " + std::to_string(i);

            using namespace std::chrono_literals;
            env.app().cluster().update(
                kp.first,
                name,
                200,
                env.timeKeeper().now() - 10s);
            nodes.insert( std::make_pair(
                toBase58(TokenType::NodePublic, kp.first), name));
        }

        peers = env.rpc("peers")[jss::result];
        if(! BEAST_EXPECT(peers.isMember(jss::cluster)))
            return;
        if(! BEAST_EXPECT(peers[jss::cluster].size() == nodes.size()))
            return;
        for(auto it  = peers[jss::cluster].begin();
                 it != peers[jss::cluster].end();
                 ++it)
        {
            auto key = it.key().asString();
            auto search = nodes.find(key);
            if(! BEAST_EXPECTS(search != nodes.end(), key))
                continue;
            if(! BEAST_EXPECT((*it).isMember(jss::tag)))
                continue;
            auto tag = (*it)[jss::tag].asString();
            BEAST_EXPECTS((*it)[jss::tag].asString() == nodes[key], key);
        }
        BEAST_EXPECT(peers.isMember(jss::peers) && peers[jss::peers].isNull());
    }

public:
    void run () override
    {
        testRequest();
    }
};

BEAST_DEFINE_TESTSUITE (Peers, rpc, ripple);

}  
























