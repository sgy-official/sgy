

#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/TxFlags.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

namespace RPC {

class AccountLinesRPC_test : public beast::unit_test::suite
{
public:
    void testAccountLines()
    {
        testcase ("acccount_lines");

        using namespace test::jtx;
        Env env(*this);
        {
            auto const lines = env.rpc ("json", "account_lines", "{ }");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::missing_field_error(jss::account)[jss::error_message]);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
        }
        Account const alice {"alice"};
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcACT_NOT_FOUND)[jss::error_message]);
        }
        env.fund(XRP(10000), alice);
        env.close();
        LedgerInfo const ledger3Info = env.closed()->info();
        BEAST_EXPECT(ledger3Info.seq == 3);

        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 0);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": "nonsense"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                "ledgerIndexMalformed");
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": 50000})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                "ledgerNotFound");
        }
        Account const gw1 {"gw1"};
        env.fund(XRP(10000), gw1);
        std::vector<IOU> gw1Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
            gw1Currencies.push_back(
                gw1[std::string("YA") + static_cast<char>('A' + c)]);
            IOU const& gw1Currency = gw1Currencies.back();

            env(trust(alice, gw1Currency(100 + c)));
            env(pay(gw1, alice, gw1Currency(50 + c)));
        }
        env.close();
        LedgerInfo const ledger4Info = env.closed()->info();
        BEAST_EXPECT(ledger4Info.seq == 4);

        Account const gw2 {"gw2"};
        env.fund(XRP(10000), gw2);

        env(fset(gw2, asfRequireAuth));
        env.close();
        std::vector<IOU> gw2Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
            gw2Currencies.push_back(
                gw2[std::string("ZA") + static_cast<char>('A' + c)]);
            IOU const& gw2Currency = gw2Currencies.back();

            env(trust(alice, gw2Currency(200 + c)));
            env(trust(gw2, gw2Currency(0), alice, tfSetfAuth));
            env.close();
            env(pay(gw2, alice, gw2Currency(100 + c)));
            env.close();

            env(trust(alice, gw2Currency(0), gw2, tfSetNoRipple | tfSetFreeze));
        }
        env.close();
        LedgerInfo const ledger58Info = env.closed()->info();
        BEAST_EXPECT(ledger58Info.seq == 58);

        auto testAccountLinesHistory =
        [this, &env](Account const& account, LedgerInfo const& info, int count)
        {
            auto const linesSeq = env.rpc ("json", "account_lines",
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_index": )" + std::to_string(info.seq) + "}");
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].size() == count);

            auto const linesHash = env.rpc ("json", "account_lines",
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(info.hash) + R"("})");
            BEAST_EXPECT(linesHash[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesHash[jss::result][jss::lines].size() == count);
        };

        testAccountLinesHistory (alice, ledger3Info, 0);

        testAccountLinesHistory (alice, ledger4Info, 26);

        testAccountLinesHistory (alice, ledger58Info, 52);

        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(ledger4Info.hash) + R"(", )"
                R"("ledger_index": )" + std::to_string(ledger58Info.seq) + "}");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 52);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": ")" + gw1.human() + R"("})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": -1})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::expected_field_message(jss::limit, "unsigned integer"));
        }
        {
            auto const linesA = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1})");
            BEAST_EXPECT(linesA[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesA[jss::result][jss::lines].size() == 1);

            auto marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 51);

            auto const linesC = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 3, )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesC[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesC[jss::result][jss::lines].size() == 3);

            marker[5] = marker[5] == '7' ? '8' : '7';
            auto const linesD = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("})");
            BEAST_EXPECT(linesD[jss::result][jss::error_message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": true})");
            BEAST_EXPECT(lines[jss::result][jss::error_message] ==
                RPC::expected_field_message(jss::marker, "string"));
        }
        {
            auto const lines = env.rpc ("json", "account_lines",
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + gw2.human() + R"("})");
            auto const& line = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line[jss::freeze].asBool() == true);
            BEAST_EXPECT(line[jss::no_ripple].asBool() == true);
            BEAST_EXPECT(line[jss::peer_authorized].asBool() == true);
        }
        {
            auto const linesA = env.rpc ("json", "account_lines",
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + alice.human() + R"("})");
            auto const& lineA = linesA[jss::result][jss::lines][0u];
            BEAST_EXPECT(lineA[jss::freeze_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::no_ripple_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::authorized].asBool() == true);

            BEAST_EXPECT(linesA[jss::result].isMember(jss::marker));
            auto const marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json", "account_lines",
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 25, )"
                R"("marker": ")" + marker + R"(", )"
                R"("peer": ")" + alice.human() + R"("})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 25);
            BEAST_EXPECT(! linesB[jss::result].isMember(jss::marker));
        }
    }

    void testAccountLineDelete()
    {
        testcase ("Entry pointed to by marker is removed");
        using namespace test::jtx;
        Env env(*this);

        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const cheri {"cheri"};
        Account const gw1 {"gw1"};
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), alice, becky, cheri, gw1, gw2);
        env.close();

        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];
        env(trust(alice, USD(200)));
        env(trust(becky, EUR(200)));
        env(trust(cheri, EUR(200)));
        env.close();

        env(pay(gw2, becky, EUR(100)));
        env.close();

        env(offer(alice, EUR(100), XRP(100)));
        env.close();

        env(offer(becky, XRP(100), EUR(100)));
        env.close();

        auto const linesBeg = env.rpc ("json", "account_lines",
            R"({"account": ")" + alice.human() + R"(", )"
            R"("limit": 1})");
        BEAST_EXPECT(linesBeg[jss::result][jss::lines][0u][jss::currency] == "USD");
        BEAST_EXPECT(linesBeg[jss::result].isMember(jss::marker));

        env(pay(alice, cheri, EUR(100)));
        env.close();

        auto const linesEnd = env.rpc ("json", "account_lines",
            R"({"account": ")" + alice.human() + R"(", )"
            R"("marker": ")" +
            linesBeg[jss::result][jss::marker].asString() + R"("})");
        BEAST_EXPECT(linesEnd[jss::result][jss::error_message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
    }

    void testAccountLines2()
    {
        testcase ("V2: acccount_lines");

        using namespace test::jtx;
        Env env(*this);
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0")"
                " }");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5)"
                " }");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::missing_field_error(jss::account)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        Account const alice {"alice"};
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcACT_NOT_FOUND)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        env.fund(XRP(10000), alice);
        env.close();
        LedgerInfo const ledger3Info = env.closed()->info();
        BEAST_EXPECT(ledger3Info.seq == 3);

        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 0);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": "nonsense"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                "ledgerIndexMalformed");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_index": 50000}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                "ledgerNotFound");
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        Account const gw1 {"gw1"};
        env.fund(XRP(10000), gw1);
        std::vector<IOU> gw1Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
            gw1Currencies.push_back(
                gw1[std::string("YA") + static_cast<char>('A' + c)]);
            IOU const& gw1Currency = gw1Currencies.back();

            env(trust(alice, gw1Currency(100 + c)));
            env(pay(gw1, alice, gw1Currency(50 + c)));
        }
        env.close();
        LedgerInfo const ledger4Info = env.closed()->info();
        BEAST_EXPECT(ledger4Info.seq == 4);

        Account const gw2 {"gw2"};
        env.fund(XRP(10000), gw2);

        env(fset(gw2, asfRequireAuth));
        env.close();
        std::vector<IOU> gw2Currencies;

        for (char c = 0; c <= ('Z' - 'A'); ++c)
        {
            gw2Currencies.push_back(
                gw2[std::string("ZA") + static_cast<char>('A' + c)]);
            IOU const& gw2Currency = gw2Currencies.back();

            env(trust(alice, gw2Currency(200 + c)));
            env(trust(gw2, gw2Currency(0), alice, tfSetfAuth));
            env.close();
            env(pay(gw2, alice, gw2Currency(100 + c)));
            env.close();

            env(trust(alice, gw2Currency(0), gw2, tfSetNoRipple | tfSetFreeze));
        }
        env.close();
        LedgerInfo const ledger58Info = env.closed()->info();
        BEAST_EXPECT(ledger58Info.seq == 58);

        auto testAccountLinesHistory =
        [this, &env](Account const& account, LedgerInfo const& info, int count)
        {
            auto const linesSeq = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_index": )" + std::to_string(info.seq) + "}}");
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesSeq[jss::result][jss::lines].size() == count);
            BEAST_EXPECT(linesSeq.isMember(jss::jsonrpc) && linesSeq[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesSeq.isMember(jss::ripplerpc) && linesSeq[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesSeq.isMember(jss::id) && linesSeq[jss::id] == 5);

            auto const linesHash = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + account.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(info.hash) + R"("}})");
            BEAST_EXPECT(linesHash[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesHash[jss::result][jss::lines].size() == count);
            BEAST_EXPECT(linesHash.isMember(jss::jsonrpc) && linesHash[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesHash.isMember(jss::ripplerpc) && linesHash[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesHash.isMember(jss::id) && linesHash[jss::id] == 5);
        };

        testAccountLinesHistory (alice, ledger3Info, 0);

        testAccountLinesHistory (alice, ledger4Info, 26);

        testAccountLinesHistory (alice, ledger58Info, 52);

        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("ledger_hash": ")" + to_string(ledger4Info.hash) + R"(", )"
                R"("ledger_index": )" + std::to_string(ledger58Info.seq) + "}}");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 52);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": ")" + gw1.human() + R"("}})");
            BEAST_EXPECT(lines[jss::result][jss::lines].isArray());
            BEAST_EXPECT(lines[jss::result][jss::lines].size() == 26);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("peer": )"
                R"("n9MJkEKHDhy5eTLuHUQeAAjo382frHNbFK4C8hcwN4nwM2SrLdBj"}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::make_error(rpcBAD_SEED)[jss::error_message]);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": -1}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::expected_field_message(jss::limit, "unsigned integer"));
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const linesA = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1}})");
            BEAST_EXPECT(linesA[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesA[jss::result][jss::lines].size() == 1);
            BEAST_EXPECT(linesA.isMember(jss::jsonrpc) && linesA[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::ripplerpc) && linesA[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::id) && linesA[jss::id] == 5);

            auto marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 51);
            BEAST_EXPECT(linesB.isMember(jss::jsonrpc) && linesB[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::ripplerpc) && linesB[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::id) && linesB[jss::id] == 5);

            auto const linesC = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 3, )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesC[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesC[jss::result][jss::lines].size() == 3);
            BEAST_EXPECT(linesC.isMember(jss::jsonrpc) && linesC[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesC.isMember(jss::ripplerpc) && linesC[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesC.isMember(jss::id) && linesC[jss::id] == 5);

            marker[5] = marker[5] == '7' ? '8' : '7';
            auto const linesD = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": ")" + marker + R"("}})");
            BEAST_EXPECT(linesD[jss::error][jss::message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
            BEAST_EXPECT(linesD.isMember(jss::jsonrpc) && linesD[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesD.isMember(jss::ripplerpc) && linesD[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesD.isMember(jss::id) && linesD[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("marker": true}})");
            BEAST_EXPECT(lines[jss::error][jss::message] ==
                RPC::expected_field_message(jss::marker, "string"));
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const lines = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + alice.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + gw2.human() + R"("}})");
            auto const& line = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line[jss::freeze].asBool() == true);
            BEAST_EXPECT(line[jss::no_ripple].asBool() == true);
            BEAST_EXPECT(line[jss::peer_authorized].asBool() == true);
            BEAST_EXPECT(lines.isMember(jss::jsonrpc) && lines[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::ripplerpc) && lines[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(lines.isMember(jss::id) && lines[jss::id] == 5);
        }
        {
            auto const linesA = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 1, )"
                R"("peer": ")" + alice.human() + R"("}})");
            auto const& lineA = linesA[jss::result][jss::lines][0u];
            BEAST_EXPECT(lineA[jss::freeze_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::no_ripple_peer].asBool() == true);
            BEAST_EXPECT(lineA[jss::authorized].asBool() == true);
            BEAST_EXPECT(linesA.isMember(jss::jsonrpc) && linesA[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::ripplerpc) && linesA[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesA.isMember(jss::id) && linesA[jss::id] == 5);

            BEAST_EXPECT(linesA[jss::result].isMember(jss::marker));
            auto const marker = linesA[jss::result][jss::marker].asString();
            auto const linesB = env.rpc ("json2", "{ "
                R"("method" : "account_lines",)"
                R"("jsonrpc" : "2.0",)"
                R"("ripplerpc" : "2.0",)"
                R"("id" : 5,)"
                R"("params": )"
                R"({"account": ")" + gw2.human() + R"(", )"
                R"("limit": 25, )"
                R"("marker": ")" + marker + R"(", )"
                R"("peer": ")" + alice.human() + R"("}})");
            BEAST_EXPECT(linesB[jss::result][jss::lines].isArray());
            BEAST_EXPECT(linesB[jss::result][jss::lines].size() == 25);
            BEAST_EXPECT(! linesB[jss::result].isMember(jss::marker));
            BEAST_EXPECT(linesB.isMember(jss::jsonrpc) && linesB[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::ripplerpc) && linesB[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(linesB.isMember(jss::id) && linesB[jss::id] == 5);
        }
    }

    void testAccountLineDelete2()
    {
        testcase ("V2: account_lines with removed marker");

        using namespace test::jtx;
        Env env(*this);

        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const cheri {"cheri"};
        Account const gw1 {"gw1"};
        Account const gw2 {"gw2"};
        env.fund(XRP(10000), alice, becky, cheri, gw1, gw2);
        env.close();

        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];
        env(trust(alice, USD(200)));
        env(trust(becky, EUR(200)));
        env(trust(cheri, EUR(200)));
        env.close();

        env(pay(gw2, becky, EUR(100)));
        env.close();

        env(offer(alice, EUR(100), XRP(100)));
        env.close();

        env(offer(becky, XRP(100), EUR(100)));
        env.close();

        auto const linesBeg = env.rpc ("json2", "{ "
            R"("method" : "account_lines",)"
            R"("jsonrpc" : "2.0",)"
            R"("ripplerpc" : "2.0",)"
            R"("id" : 5,)"
            R"("params": )"
            R"({"account": ")" + alice.human() + R"(", )"
            R"("limit": 1}})");
        BEAST_EXPECT(linesBeg[jss::result][jss::lines][0u][jss::currency] == "USD");
        BEAST_EXPECT(linesBeg[jss::result].isMember(jss::marker));
        BEAST_EXPECT(linesBeg.isMember(jss::jsonrpc) && linesBeg[jss::jsonrpc] == "2.0");
        BEAST_EXPECT(linesBeg.isMember(jss::ripplerpc) && linesBeg[jss::ripplerpc] == "2.0");
        BEAST_EXPECT(linesBeg.isMember(jss::id) && linesBeg[jss::id] == 5);

        env(pay(alice, cheri, EUR(100)));
        env.close();

        auto const linesEnd = env.rpc ("json2", "{ "
            R"("method" : "account_lines",)"
            R"("jsonrpc" : "2.0",)"
            R"("ripplerpc" : "2.0",)"
            R"("id" : 5,)"
            R"("params": )"
            R"({"account": ")" + alice.human() + R"(", )"
            R"("marker": ")" +
            linesBeg[jss::result][jss::marker].asString() + R"("}})");
        BEAST_EXPECT(linesEnd[jss::error][jss::message] ==
                RPC::make_error(rpcINVALID_PARAMS)[jss::error_message]);
        BEAST_EXPECT(linesEnd.isMember(jss::jsonrpc) && linesEnd[jss::jsonrpc] == "2.0");
        BEAST_EXPECT(linesEnd.isMember(jss::ripplerpc) && linesEnd[jss::ripplerpc] == "2.0");
        BEAST_EXPECT(linesEnd.isMember(jss::id) && linesEnd[jss::id] == 5);
    }

    void run () override
    {
        testAccountLines();
        testAccountLineDelete();
        testAccountLines2();
        testAccountLineDelete2();
    }
};

BEAST_DEFINE_TESTSUITE(AccountLinesRPC,app,ripple);

} 
} 

























