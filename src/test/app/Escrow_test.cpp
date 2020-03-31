

#include <test/jtx.h>
#include <ripple/app/tx/applySteps.h>
#include <ripple/ledger/Directory.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/TxFlags.h>
#include <algorithm>
#include <iterator>

namespace ripple {
namespace test {

struct Escrow_test : public beast::unit_test::suite
{
    std::array<std::uint8_t, 4> const fb1 =
    {{
        0xA0, 0x02, 0x80, 0x00
    }};

    std::array<std::uint8_t, 39> const cb1 =
    {{
        0xA0, 0x25, 0x80, 0x20, 0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14,
        0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24, 0x27, 0xAE, 0x41, 0xE4,
        0x64, 0x9B, 0x93, 0x4C, 0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55,
        0x81, 0x01, 0x00
    }};

    std::array<std::uint8_t, 7> const fb2 =
    {{
        0xA0, 0x05, 0x80, 0x03, 0x61, 0x61, 0x61
    }};

    std::array<std::uint8_t, 39> const cb2 =
    {{
        0xA0, 0x25, 0x80, 0x20, 0x98, 0x34, 0x87, 0x6D, 0xCF, 0xB0, 0x5C, 0xB1,
        0x67, 0xA5, 0xC2, 0x49, 0x53, 0xEB, 0xA5, 0x8C, 0x4A, 0xC8, 0x9B, 0x1A,
        0xDF, 0x57, 0xF2, 0x8F, 0x2F, 0x9D, 0x09, 0xAF, 0x10, 0x7E, 0xE8, 0xF0,
        0x81, 0x01, 0x03
    }};

    std::array<std::uint8_t, 8> const fb3 =
    {{
        0xA0, 0x06, 0x80, 0x04, 0x6E, 0x69, 0x6B, 0x62
    }};

    std::array<std::uint8_t, 39> const cb3 =
    {{
        0xA0, 0x25, 0x80, 0x20, 0x6E, 0x4C, 0x71, 0x45, 0x30, 0xC0, 0xA4, 0x26,
        0x8B, 0x3F, 0xA6, 0x3B, 0x1B, 0x60, 0x6F, 0x2D, 0x26, 0x4A, 0x2D, 0x85,
        0x7B, 0xE8, 0xA0, 0x9C, 0x1D, 0xFD, 0x57, 0x0D, 0x15, 0x85, 0x8B, 0xD4,
        0x81, 0x01, 0x04
    }};

    
    struct finish_time
    {
    private:
        NetClock::time_point value_;

    public:
        explicit
        finish_time (NetClock::time_point const& value)
            : value_ (value)
        {
        }

        void
        operator()(jtx::Env&, jtx::JTx& jt) const
        {
            jt.jv[sfFinishAfter.jsonName] = value_.time_since_epoch().count();
        }
    };

    
    struct cancel_time
    {
    private:
        NetClock::time_point value_;

    public:
        explicit
        cancel_time (NetClock::time_point const& value)
            : value_ (value)
        {
        }

        void
        operator()(jtx::Env&, jtx::JTx& jt) const
        {
            jt.jv[sfCancelAfter.jsonName] = value_.time_since_epoch().count();
        }
    };

    struct condition
    {
    private:
        std::string value_;

    public:
        explicit
        condition (Slice condition)
            : value_ (strHex(condition))
        {
        }

        template <size_t N>
        explicit condition (std::array<std::uint8_t, N> c)
            : condition(makeSlice(c))
        {
        }

        void
        operator()(jtx::Env&, jtx::JTx& jt) const
        {
            jt.jv[sfCondition.jsonName] = value_;
        }
    };

    struct fulfillment
    {
    private:
        std::string value_;

    public:
        explicit
        fulfillment (Slice condition)
            : value_ (strHex(condition))
        {
        }

        template <size_t N>
        explicit fulfillment (std::array<std::uint8_t, N> f)
            : fulfillment(makeSlice(f))
        {
        }

        void
        operator()(jtx::Env&, jtx::JTx& jt) const
        {
            jt.jv[sfFulfillment.jsonName] = value_;
        }
    };

    static
    Json::Value
    escrow (
        jtx::Account const& account, jtx::Account const& to, STAmount const& amount)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::EscrowCreate;
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human();
        jv[jss::Destination] = to.human();
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        return jv;
    }

    static
    Json::Value
    finish (jtx::Account const& account,
        jtx::Account const& from, std::uint32_t seq)
    {
        Json::Value jv;
        jv[jss::TransactionType] = jss::EscrowFinish;
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human();
        jv[sfOwner.jsonName] = from.human();
        jv[sfOfferSequence.jsonName] = seq;
        return jv;
    }

    static
    Json::Value
    cancel (jtx::Account const& account,
        jtx::Account const& from, std::uint32_t seq)
    {
        Json::Value jv;
        jv[jss::TransactionType] = jss::EscrowCancel;
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human();
        jv[sfOwner.jsonName] = from.human();
        jv[sfOfferSequence.jsonName] = seq;
        return jv;
    }

    void
    testEnablement()
    {
        testcase ("Enablement");

        using namespace jtx;
        using namespace std::chrono;

        { 
            Env env(*this, supported_amendments() - featureEscrow);
            env.fund(XRP(5000), "alice", "bob");
            env(escrow("alice", "bob", XRP(1000)),
                finish_time(env.now() + 1s),        ter(temDISABLED));
            env(finish("bob", "alice", 1),          ter(temDISABLED));
            env(cancel("bob", "alice", 1),          ter(temDISABLED));
        }

        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            env(escrow("alice", "bob", XRP(1000)), finish_time(env.now() + 1s));
            env.close();

            auto const seq1 = env.seq("alice");

            env(escrow("alice", "bob", XRP(1000)), condition (cb1),
                finish_time(env.now() + 1s), fee(1500));
            env.close();
            env(finish("bob", "alice", seq1),
                condition(cb1), fulfillment(fb1), fee(1500));

            auto const seq2 = env.seq("alice");

            env(escrow("alice", "bob", XRP(1000)), condition(cb2),
                finish_time(env.now() + 1s), cancel_time(env.now() + 2s), fee(1500));
            env.close();
            env(cancel("bob", "alice", seq2), fee(1500));
        }
    }

    void
    testTiming()
    {
        using namespace jtx;
        using namespace std::chrono;

        {
            testcase("Timing: Finish Only");
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            env.close();

            auto const ts = env.now() + 97s;

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(ts));

            for ( ; env.now() < ts; env.close())
                env(finish("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));

            env(finish("bob", "alice", seq), fee(1500));
        }

        {
            testcase("Timing: Cancel Only");
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            env.close();

            auto const ts = env.now() + 117s;

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), condition(cb1), cancel_time(ts));

            for ( ; env.now() < ts; env.close())
                env(cancel("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));

            env(finish("bob", "alice", seq), condition(cb1),
                fulfillment(fb1), fee(1500), ter(tecNO_PERMISSION));

            env(cancel("bob", "alice", seq), fee(1500));
        }

        {
            testcase("Timing: Finish and Cancel -> Finish");
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            env.close();

            auto const fts = env.now() + 117s;
            auto const cts = env.now() + 192s;

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(fts), cancel_time(cts));

            for ( ; env.now() < fts; env.close())
            {
                env(finish("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));
                env(cancel("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));
            }

            env(cancel("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));

            env(finish("bob", "alice", seq), fee(1500));
        }

        {
            testcase("Timing: Finish and Cancel -> Cancel");
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            env.close();

            auto const fts = env.now() + 109s;
            auto const cts = env.now() + 184s;

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(fts), cancel_time(cts));

            for ( ; env.now() < fts; env.close())
            {
                env(finish("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));
                env(cancel("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));
            }

            for ( ; env.now() < cts; env.close())
                env(cancel("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));

            env(finish("bob", "alice", seq), fee(1500), ter(tecNO_PERMISSION));

            env(cancel("bob", "alice", seq), fee(1500));
        }
    }

    void
    testTags()
    {
        testcase ("Tags");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);

        auto const alice = Account("alice");
        auto const bob = Account("bob");

        env.fund(XRP(5000), alice, bob);

        env(fset(bob, asfRequireDest));
        env(escrow(alice, bob, XRP(1000)), finish_time(env.now() + 1s), ter(tecDST_TAG_NEEDED));

        auto const seq = env.seq(alice);

        env(escrow(alice, bob, XRP(1000)), finish_time(env.now() + 1s), stag(1), dtag(2));

        auto const sle = env.le(keylet::escrow(alice.id(), seq));
        BEAST_EXPECT(sle);
        BEAST_EXPECT((*sle)[sfSourceTag] == 1);
        BEAST_EXPECT((*sle)[sfDestinationTag] == 2);
    }

    void
    testDisallowXRP()
    {
        testcase ("Disallow XRP");

        using namespace jtx;
        using namespace std::chrono;

        {
            Env env(*this, supported_amendments() - featureDepositAuth);

            env.fund(XRP(5000), "bob", "george");
            env(fset("george", asfDisallowXRP));
            env(escrow("bob", "george", XRP(10)), finish_time(env.now() + 1s),
                ter (tecNO_TARGET));
        }
        {
            Env env(*this);

            env.fund(XRP(5000), "bob", "george");
            env(fset("george", asfDisallowXRP));
            env(escrow("bob", "george", XRP(10)), finish_time(env.now() + 1s));
        }
    }

    void
    test1571()
    {
        using namespace jtx;
        using namespace std::chrono;

        {
            testcase ("Implied Finish Time (without fix1571)");

            Env env(*this, supported_amendments() - fix1571);
            env.fund(XRP(5000), "alice", "bob", "carol");
            env.close();

            auto const seq1 = env.seq("alice");
            env(escrow("alice", "bob", XRP(100)), cancel_time(env.now() + 1s), fee(1500));
            env.close();
            env(finish("carol", "alice", seq1), fee(1500));
            BEAST_EXPECT (env.balance ("bob") == XRP(5100));

            env.close();

            auto const seq2 = env.seq("alice");
            env(escrow("alice", "bob", XRP(100)), cancel_time(env.now() + 1s),
                condition(cb1), fee(1500));
            env.close();
            env(finish("carol", "alice", seq2), condition(cb1),
                fulfillment(fb1), fee(1500));
            BEAST_EXPECT (env.balance ("bob") == XRP(5200));
        }

        {
            testcase ("Implied Finish Time (with fix1571)");

            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");
            env.close();

            env (escrow("alice", "bob", XRP(100)), cancel_time(env.now() + 90s),
                fee(1500), ter(temMALFORMED));

            auto const seq = env.seq("alice");
            env (escrow("alice", "bob", XRP(100)), cancel_time(env.now() + 90s),
                condition(cb1), fee(1500));
            env.close();
            env (finish("carol", "alice", seq), condition(cb1),
                fulfillment(fb1), fee(1500));
            BEAST_EXPECT (env.balance ("bob") == XRP(5100));
        }
    }

    void
    testFails()
    {
        testcase ("Failure Cases");

        using namespace jtx;
        using namespace std::chrono;

        Env env(*this);
        env.fund(XRP(5000), "alice", "bob");
        env.close();

        env(escrow("alice", "bob", XRP(1000)),
            finish_time(env.now() - 5s), ter(tecNO_PERMISSION));

        env(escrow("alice", "bob", XRP(1000)), condition(cb1),
            cancel_time(env.now() - 5s), ter(tecNO_PERMISSION));

        env(escrow("alice", "carol", XRP(1000)),
            finish_time(env.now() + 1s), ter(tecNO_DST));

        env.fund(XRP(5000), "carol");

        env (escrow("alice", "carol", Account("alice")["USD"](500)),
            finish_time(env.now() + 1s), ter(temBAD_AMOUNT));

        env (escrow("alice", "carol", XRP(0)),
            finish_time(env.now() + 1s), ter(temBAD_AMOUNT));
        env (escrow("alice", "carol", XRP(-1000)),
            finish_time(env.now() + 1s), ter(temBAD_AMOUNT));

        env (escrow("alice", "carol", XRP(1)), ter(temBAD_EXPIRATION));

        env (escrow("alice", "carol", XRP(1)),
            cancel_time(env.now() + 1s), ter(temMALFORMED));

        env (escrow("alice", "carol", XRP(1)),
            finish_time(env.now() - 1s), ter(tecNO_PERMISSION));

        env(escrow("alice", "carol", XRP(1)), condition(cb1),
            finish_time(env.now() + 10s), cancel_time(env.now() + 10s), ter(temBAD_EXPIRATION));

        env(escrow("alice", "carol", XRP(1)), condition(cb1),
            finish_time(env.now() + 10s), cancel_time(env.now() + 5s), ter(temBAD_EXPIRATION));

        env(fset("carol", asfRequireDest));

        env(escrow("alice", "carol", XRP(1)),
            condition(cb1), cancel_time(env.now() + 1s), ter(tecDST_TAG_NEEDED));

        env(escrow("alice", "carol", XRP(1)),
            condition(cb1), cancel_time(env.now() + 1s), dtag(1));

        { 
            auto const accountReserve = drops(env.current()->fees().reserve);
            auto const accountIncrement = drops(env.current()->fees().increment);

            env.fund (accountReserve + accountIncrement + XRP(50), "daniel");
            env(escrow("daniel", "bob", XRP(51)),
                finish_time(env.now() + 1s), ter(tecUNFUNDED));

            env.fund (accountReserve + accountIncrement + XRP(50), "evan");
            env(escrow("evan", "bob", XRP(50)),
                finish_time(env.now() + 1s), ter(tecUNFUNDED));

            env.fund (accountReserve, "frank");
            env(escrow("frank", "bob", XRP(1)),
                finish_time(env.now() + 1s), ter(tecINSUFFICIENT_RESERVE));
        }

        { 
            env.fund (XRP(5000), "hannah");
            auto const seq = env.seq("hannah");
            env(escrow("hannah", "hannah", XRP(10)),
                finish_time(env.now() + 1s), fee(1500));
            env.close();
            env(finish ("hannah", "hannah", seq + 7), fee(1500), ter(tecNO_TARGET));
        }

        { 
            env.fund (XRP(5000), "ivan");
            auto const seq = env.seq("ivan");

            env(escrow("ivan", "ivan", XRP(10)), finish_time(env.now() + 1s));
            env.close();
            env(finish("ivan", "ivan", seq), condition(cb1), fulfillment(fb1),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
        }
    }

    void
    testLockup()
    {
        testcase ("Lockup");

        using namespace jtx;
        using namespace std::chrono;

        {
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            auto const seq = env.seq("alice");
            env(escrow("alice", "alice", XRP(1000)), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));

            env(cancel("bob", "alice", seq),                ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq),                ter(tecNO_PERMISSION));
            env.close();

            env(cancel("bob", "alice", seq),                ter(tecNO_PERMISSION));

            env(finish("bob", "alice", seq));
            env.require(balance("alice", XRP(5000) - drops(10)));
        }
        {
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "zelda");
            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));

            env(cancel("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env.close();

            env(cancel("zelda", "alice", seq),               ter(tecNO_PERMISSION));

            env(finish("zelda", "alice", seq));
            env.close();

            env.require(balance("alice", XRP(4000) - drops(10)));
            env.require(balance("bob", XRP(6000)));
            env.require(balance("zelda", XRP(5000) - drops(40)));
        }
        {
            Env env(*this);

            env.fund(XRP(5000), "alice", "bob", "zelda");
            env(fset ("bob", asfDepositAuth));
            env.close();

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));

            env(cancel("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env(finish("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env.close();

            env(cancel("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env(finish("zelda", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq));
            env.close();

            auto const baseFee = env.current()->fees().base;
            env.require(balance("alice", XRP(4000) - (baseFee * 5)));
            env.require(balance("bob", XRP(6000) - (baseFee * 5)));
            env.require(balance("zelda", XRP(5000) - (baseFee * 4)));
        }
        {
            Env env(*this);

            env.fund(XRP(5000), "alice", "bob", "zelda");
            env(fset ("bob", asfDepositAuth));
            env.close();
            env(deposit::auth ("bob", "zelda"));
            env.close();

            auto const seq = env.seq("alice");
            env(escrow("alice", "bob", XRP(1000)), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));
            env.close();

            env(finish("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("zelda", "alice", seq));
            env.close();

            auto const baseFee = env.current()->fees().base;
            env.require(balance("alice", XRP(4000) - (baseFee * 2)));
            env.require(balance("bob", XRP(6000) - (baseFee * 2)));
            env.require(balance("zelda", XRP(5000) - (baseFee * 1)));
        }
        {
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");
            auto const seq = env.seq("alice");
            env(escrow("alice", "alice", XRP(1000)), condition(cb2), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));

            env(cancel("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env(finish("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(finish("alice", "alice", seq),
                condition(cb2), fulfillment(fb2), fee(1500), ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq),
                condition(cb2), fulfillment(fb2), fee(1500), ter(tecNO_PERMISSION));
            env.close();

            env(cancel("alice", "alice", seq),               ter(tecNO_PERMISSION));
            env(cancel("bob", "alice", seq),                 ter(tecNO_PERMISSION));
            env(finish("bob", "alice", seq),                 ter(tecCRYPTOCONDITION_ERROR));
            env(finish("alice", "alice", seq),               ter(tecCRYPTOCONDITION_ERROR));
            env.close();

            env(finish("bob", "alice", seq),
                condition(cb2), fulfillment(fb2), fee(1500));
        }
        {
            Env env(*this);

            env.fund(XRP(5000), "alice", "bob");
            auto const seq = env.seq("alice");
            env(escrow("alice", "alice", XRP(1000)), condition(cb3), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));
            env.close();

            env(finish("bob", "alice", seq),                ter(tecCRYPTOCONDITION_ERROR));
            env(finish("alice", "alice", seq),              ter(tecCRYPTOCONDITION_ERROR));

            env(fset ("alice", asfDepositAuth));
            env.close();

            env(finish("alice", "alice", seq), condition(cb2),
                fulfillment(fb2), fee(1500),                ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq),   condition(cb3),
                fulfillment(fb3), fee(1500),                ter(tecNO_PERMISSION));
            env(finish("alice", "alice", seq), condition(cb3),
                fulfillment(fb3), fee(1500));
        }
        {
            Env env(*this);

            env.fund(XRP(5000), "alice", "bob", "zelda");
            auto const seq = env.seq("alice");
            env(escrow("alice", "alice", XRP(1000)), condition(cb3), finish_time(env.now() + 5s));
            env.require(balance("alice", XRP(4000) - drops(10)));
            env.close();

            env(deposit::auth("alice", "zelda"));
            env.close();

            env(finish("alice", "alice", seq),              ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq),                ter(tecCRYPTOCONDITION_ERROR));
            env(finish("zelda", "alice", seq),              ter(tecCRYPTOCONDITION_ERROR));

            env(fset ("alice", asfDepositAuth));
            env.close();

            env(finish("alice", "alice", seq), condition(cb2),
                fulfillment(fb2), fee(1500),                ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq),   condition(cb3),
                fulfillment(fb3), fee(1500),                ter(tecNO_PERMISSION));
            env(finish("zelda", "alice", seq), condition(cb3),
                fulfillment(fb3), fee(1500));
        }
    }

    void
    testEscrowConditions()
    {
        testcase ("Escrow with CryptoConditions");

        using namespace jtx;
        using namespace std::chrono;

        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");
            auto const seq = env.seq("alice");
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 0);
            env(escrow("alice", "carol", XRP(1000)), condition(cb1),
                cancel_time(env.now() + 1s));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env.require(balance("alice", XRP(4000) - drops(10)));
            env.require(balance("carol", XRP(5000)));
            env(cancel("bob", "alice", seq), ter(tecNO_PERMISSION));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);

            env(finish("bob", "alice", seq), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);

            env(finish("bob", "alice", seq), condition(cb1),
                fulfillment(cb1), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env(finish("bob", "alice", seq), condition(cb1),
                fulfillment(cb2), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env(finish("bob", "alice", seq), condition(cb1),
                fulfillment(cb3), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);

            env(finish("bob", "alice", seq), condition(cb2),
                fulfillment(fb1), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env(finish("bob", "alice", seq), condition(cb2),
                fulfillment(fb2), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env(finish("bob", "alice", seq), condition(cb2),
                fulfillment(fb3), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);

            env(finish("bob", "alice", seq), condition(cb1),
                fulfillment(fb1), fee(1500));

            BEAST_EXPECT(! env.le(keylet::escrow(Account("alice").id(), seq)));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 0);
            env.require(balance("carol", XRP(6000)));
            env(cancel("bob", "alice", seq), ter(tecNO_TARGET));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 0);
            env(cancel("bob", "carol", 1), ter(tecNO_TARGET));
        }
        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");
            auto const seq = env.seq("alice");
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 0);
            env(escrow("alice", "carol", XRP(1000)), condition(cb2),
                cancel_time(env.now() + 1s));
            env.close();
            env.require(balance("alice", XRP(4000) - drops(10)));
            env(cancel("bob", "alice", seq));
            env.require(balance("alice", XRP(5000) - drops(10)));
            BEAST_EXPECT(! env.le(keylet::escrow(Account("alice").id(), seq)));
        }
        {
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");
            env.close();
            auto const seq = env.seq("alice");
            env(escrow("alice", "carol", XRP(1000)), condition(cb3),
                cancel_time(env.now() + 1s));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env(cancel("bob", "alice", seq), ter(tecNO_PERMISSION));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env.close();
            env(finish("bob", "alice", seq), condition(cb3),
                fulfillment(fb3), fee(1500), ter(tecNO_PERMISSION));
            BEAST_EXPECT((*env.le("alice"))[sfOwnerCount] == 1);
            env.require(balance("carol", XRP(5000)));
        }
        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");

            std::vector<std::uint8_t> v;
            v.resize(cb1.size() + 2, 0x78);
            std::memcpy (v.data() + 1, cb1.data(), cb1.size());

            auto const p = v.data();
            auto const s = v.size();

            auto const ts = env.now() + 1s;

            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p, s}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p, s - 1}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p, s - 2}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p + 1, s - 1}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p + 1, s - 3}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p + 2, s - 2}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p + 2, s - 3}),
                cancel_time(ts), ter(temMALFORMED));

            auto const seq = env.seq("alice");
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{p + 1, s - 2}),
                cancel_time(ts), fee(100));
            env(finish("bob", "alice", seq),
                condition(cb1), fulfillment(fb1), fee(1500));
            env.require(balance("alice", XRP(4000) - drops(100)));
            env.require(balance("bob", XRP(5000) - drops(1500)));
            env.require(balance("carol", XRP(6000)));
        }
        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");

            std::vector<std::uint8_t> cv;
            cv.resize(cb2.size() + 2, 0x78);
            std::memcpy (cv.data() + 1, cb2.data(), cb2.size());

            auto const cp = cv.data();
            auto const cs = cv.size();

            std::vector<std::uint8_t> fv;
            fv.resize(fb2.size() + 2, 0x13);
            std::memcpy(fv.data() + 1, fb2.data(), fb2.size());

            auto const fp = fv.data();
            auto const fs = fv.size();

            auto const ts = env.now() + 1s;

            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp, cs}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp, cs - 1}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp, cs - 2}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp + 1, cs - 1}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp + 1, cs - 3}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp + 2, cs - 2}),
                cancel_time(ts), ter(temMALFORMED));
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp + 2, cs - 3}),
                cancel_time(ts), ter(temMALFORMED));

            auto const seq = env.seq("alice");
            env(escrow("alice", "carol", XRP(1000)), condition(Slice{cp + 1, cs - 2}),
                cancel_time(ts), fee(100));

            env(finish("bob", "alice", seq), condition(Slice{cp, cs}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp, cs - 1}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp, cs - 2}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 1}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 3}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 2, cs - 2}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 2, cs - 3}), fulfillment(Slice{fp, fs}),
                fee(1500), ter(tecCRYPTOCONDITION_ERROR));

            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp, fs}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp, fs - 1}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp, fs - 2}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp + 1, fs - 1}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp + 1, fs - 3}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp + 1, fs - 3}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp + 2, fs - 2}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{cp + 1, cs - 2}),
                fulfillment(Slice{fp + 2, fs - 3}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));

            env(finish("bob", "alice", seq), condition(cb2),
                fulfillment(fb2), fee(1500));
            env.require(balance("alice", XRP(4000) - drops(100)));
            env.require(balance("carol", XRP(6000)));
        }
        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob", "carol");

            env(escrow("alice", "carol", XRP(1000)), condition(Slice{}),
                cancel_time(env.now() + 1s), ter(temMALFORMED));

            auto const seq = env.seq("alice");
            env(escrow("alice", "carol", XRP(1000)),
                condition(cb3), cancel_time(env.now() + 1s));

            env(finish("bob", "alice", seq), condition(Slice{}),
                fulfillment(Slice{}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(cb3),
                fulfillment(Slice{}), fee(1500), ter(tecCRYPTOCONDITION_ERROR));
            env(finish("bob", "alice", seq), condition(Slice{}),
                fulfillment(fb3), fee(1500), ter(tecCRYPTOCONDITION_ERROR));

            env (finish("bob", "alice", seq), condition(cb3), ter(temMALFORMED));
            env (finish("bob", "alice", seq), fulfillment(fb3), ter(temMALFORMED));

            env(finish("bob", "alice", seq), condition(cb3),
                fulfillment(fb3), fee(1500));
            env.require(balance ("carol", XRP(6000)));
            env.require(balance ("alice", XRP(4000) - drops(10)));
        }
        { 
            Env env(*this);
            env.fund(XRP(5000), "alice", "bob");

            std::array<std::uint8_t, 45> cb =
            {{
                0xA2, 0x2B, 0x80, 0x20, 0x42, 0x4A, 0x70, 0x49, 0x49, 0x52,
                0x92, 0x67, 0xB6, 0x21, 0xB3, 0xD7, 0x91, 0x19, 0xD7, 0x29,
                0xB2, 0x38, 0x2C, 0xED, 0x8B, 0x29, 0x6C, 0x3C, 0x02, 0x8F,
                0xA9, 0x7D, 0x35, 0x0F, 0x6D, 0x07, 0x81, 0x03, 0x06, 0x34,
                0xD2, 0x82, 0x02, 0x03, 0xC8
            }};

            env(escrow("alice", "bob", XRP(1000)), condition(cb),
                cancel_time(env.now() + 1s), ter(temMALFORMED));
        }
    }

    void
    testMetaAndOwnership()
    {
        using namespace jtx;
        using namespace std::chrono;

        auto const alice = Account("alice");
        auto const bruce = Account("bruce");
        auto const carol = Account("carol");

        {
            testcase ("Metadata & Ownership (without fix1523)");
            Env env(*this, supported_amendments() - fix1523);
            env.fund(XRP(5000), alice, bruce, carol);

            auto const seq = env.seq(alice);
            env(escrow(alice, carol, XRP(1000)), finish_time(env.now() + 1s));

            BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                static_cast<std::uint8_t>(tesSUCCESS));

            auto const escrow = env.le(keylet::escrow(alice.id(), seq));
            BEAST_EXPECT(escrow);

            ripple::Dir aod (*env.current(), keylet::ownerDir(alice.id()));
            BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 1);
            BEAST_EXPECT(std::find(aod.begin(), aod.end(), escrow) != aod.end());

            ripple::Dir cod (*env.current(), keylet::ownerDir(carol.id()));
            BEAST_EXPECT(cod.begin() == cod.end());
        }
        {
            testcase ("Metadata (with fix1523, to self)");

            Env env(*this);
            env.fund(XRP(5000), alice, bruce, carol);
            auto const aseq = env.seq(alice);
            auto const bseq = env.seq(bruce);

            env(escrow(alice, alice, XRP(1000)),
                finish_time(env.now() + 1s), cancel_time(env.now() + 500s));
            BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                static_cast<std::uint8_t>(tesSUCCESS));
            env.close(5s);
            auto const aa = env.le(keylet::escrow(alice.id(), aseq));
            BEAST_EXPECT(aa);

            {
                ripple::Dir aod(*env.current(), keylet::ownerDir(alice.id()));
                BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 1);
                BEAST_EXPECT(std::find(aod.begin(), aod.end(), aa) != aod.end());
            }

            env(escrow(bruce, bruce, XRP(1000)),
                finish_time(env.now() + 1s), cancel_time(env.now() + 2s));
            BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                static_cast<std::uint8_t>(tesSUCCESS));
            env.close(5s);
            auto const bb = env.le(keylet::escrow(bruce.id(), bseq));
            BEAST_EXPECT(bb);

            {
                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 1);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bb) != bod.end());
            }

            env.close(5s);
            env(finish(alice, alice, aseq));
            {
                BEAST_EXPECT(!env.le(keylet::escrow(alice.id(), aseq)));
                BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                    static_cast<std::uint8_t>(tesSUCCESS));

                ripple::Dir aod(*env.current(), keylet::ownerDir(alice.id()));
                BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 0);
                BEAST_EXPECT(std::find(aod.begin(), aod.end(), aa) == aod.end());

                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 1);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bb) != bod.end());
            }

            env.close(5s);
            env(cancel(bruce, bruce, bseq));
            {
                BEAST_EXPECT(!env.le(keylet::escrow(bruce.id(), bseq)));
                BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                    static_cast<std::uint8_t>(tesSUCCESS));

                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 0);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bb) == bod.end());
            }
        }
        {
            testcase ("Metadata (with fix1523, to other)");

            Env env(*this);
            env.fund(XRP(5000), alice, bruce, carol);
            auto const aseq = env.seq(alice);
            auto const bseq = env.seq(bruce);

            env(escrow(alice, bruce, XRP(1000)), finish_time(env.now() + 1s));
            BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                static_cast<std::uint8_t>(tesSUCCESS));
            env.close(5s);
            env(escrow(bruce, carol, XRP(1000)),
                finish_time(env.now() + 1s), cancel_time(env.now() + 2s));
            BEAST_EXPECT((*env.meta())[sfTransactionResult] ==
                static_cast<std::uint8_t>(tesSUCCESS));
            env.close(5s);

            auto const ab = env.le(keylet::escrow(alice.id(), aseq));
            BEAST_EXPECT(ab);

            auto const bc = env.le(keylet::escrow(bruce.id(), bseq));
            BEAST_EXPECT(bc);

            {
                ripple::Dir aod(*env.current(), keylet::ownerDir(alice.id()));
                BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 1);
                BEAST_EXPECT(std::find(aod.begin(), aod.end(), ab) != aod.end());

                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 2);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), ab) != bod.end());
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bc) != bod.end());

                ripple::Dir cod(*env.current(), keylet::ownerDir(carol.id()));
                BEAST_EXPECT(std::distance(cod.begin(), cod.end()) == 1);
                BEAST_EXPECT(std::find(cod.begin(), cod.end(), bc) != cod.end());
            }

            env.close(5s);
            env(finish(alice, alice, aseq));
            {
                BEAST_EXPECT(!env.le(keylet::escrow(alice.id(), aseq)));
                BEAST_EXPECT(env.le(keylet::escrow(bruce.id(), bseq)));

                ripple::Dir aod(*env.current(), keylet::ownerDir(alice.id()));
                BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 0);
                BEAST_EXPECT(std::find(aod.begin(), aod.end(), ab) == aod.end());

                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 1);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), ab) == bod.end());
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bc) != bod.end());

                ripple::Dir cod(*env.current(), keylet::ownerDir(carol.id()));
                BEAST_EXPECT(std::distance(cod.begin(), cod.end()) == 1);
            }

            env.close(5s);
            env(cancel(bruce, bruce, bseq));
            {
                BEAST_EXPECT(!env.le(keylet::escrow(alice.id(), aseq)));
                BEAST_EXPECT(!env.le(keylet::escrow(bruce.id(), bseq)));

                ripple::Dir aod(*env.current(), keylet::ownerDir(alice.id()));
                BEAST_EXPECT(std::distance(aod.begin(), aod.end()) == 0);
                BEAST_EXPECT(std::find(aod.begin(), aod.end(), ab) == aod.end());

                ripple::Dir bod(*env.current(), keylet::ownerDir(bruce.id()));
                BEAST_EXPECT(std::distance(bod.begin(), bod.end()) == 0);
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), ab) == bod.end());
                BEAST_EXPECT(std::find(bod.begin(), bod.end(), bc) == bod.end());

                ripple::Dir cod(*env.current(), keylet::ownerDir(carol.id()));
                BEAST_EXPECT(std::distance(cod.begin(), cod.end()) == 0);
            }
        }
    }

    void testConsequences()
    {
        testcase ("Consequences");

        using namespace jtx;
        using namespace std::chrono;
        Env env(*this);

        env.memoize("alice");
        env.memoize("bob");
        env.memoize("carol");

        {
            auto const jtx = env.jt(escrow("alice", "carol", XRP(1000)),
                finish_time(env.now() + 1s), seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(1000));
        }

        {
            auto const jtx = env.jt(cancel("bob", "alice", 3), seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }

        {
            auto const jtx = env.jt(finish("bob", "alice", 3), seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }
    }

    void run() override
    {
        testEnablement();
        testTiming();
        testTags();
        testDisallowXRP();
        test1571();
        testFails();
        testLockup();
        testEscrowConditions();
        testMetaAndOwnership();
        testConsequences();
    }
};

BEAST_DEFINE_TESTSUITE(Escrow,app,ripple);

} 
} 
























