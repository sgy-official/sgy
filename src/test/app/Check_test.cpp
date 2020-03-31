

#include <test/jtx.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

namespace test {
namespace jtx {
namespace check {

Json::Value
create (jtx::Account const& account,
    jtx::Account const& dest, STAmount const& sendMax)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = account.human();
    jv[sfSendMax.jsonName] = sendMax.getJson(JsonOptions::none);
    jv[sfDestination.jsonName] = dest.human();
    jv[sfTransactionType.jsonName] = jss::CheckCreate;
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

struct DeliverMin
{
    STAmount value;
    explicit DeliverMin (STAmount const& deliverMin)
    : value (deliverMin) { }
};

Json::Value
cash (jtx::Account const& dest,
    uint256 const& checkId, STAmount const& amount)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfAmount.jsonName]  = amount.getJson(JsonOptions::none);
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = jss::CheckCash;
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

Json::Value
cash (jtx::Account const& dest,
    uint256 const& checkId, DeliverMin const& atLeast)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfDeliverMin.jsonName]  = atLeast.value.getJson(JsonOptions::none);
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = jss::CheckCash;
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

Json::Value
cancel (jtx::Account const& dest, uint256 const& checkId)
{
    Json::Value jv;
    jv[sfAccount.jsonName] = dest.human();
    jv[sfCheckID.jsonName] = to_string (checkId);
    jv[sfTransactionType.jsonName] = jss::CheckCancel;
    jv[sfFlags.jsonName] = tfUniversal;
    return jv;
}

} 


class expiration
{
private:
    std::uint32_t const expry_;

public:
    explicit expiration (NetClock::time_point const& expiry)
        : expry_{expiry.time_since_epoch().count()}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfExpiration.jsonName] = expry_;
    }
};


class source_tag
{
private:
    std::uint32_t const tag_;

public:
    explicit source_tag (std::uint32_t tag)
        : tag_{tag}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfSourceTag.jsonName] = tag_;
    }
};


class dest_tag
{
private:
    std::uint32_t const tag_;

public:
    explicit dest_tag (std::uint32_t tag)
        : tag_{tag}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfDestinationTag.jsonName] = tag_;
    }
};


class invoice_id
{
private:
    uint256 const id_;

public:
    explicit invoice_id (uint256 const& id)
        : id_{id}
    {
    }

    void
    operator()(Env&, JTx& jt) const
    {
        jt[sfInvoiceID.jsonName] = to_string (id_);
    }
};

} 
} 

class Check_test : public beast::unit_test::suite
{
    static std::vector<std::shared_ptr<SLE const>>
    checksOnAccount (test::jtx::Env& env, test::jtx::Account account)
    {
        std::vector<std::shared_ptr<SLE const>> result;
        forEachItem (*env.current (), account,
            [&result](std::shared_ptr<SLE const> const& sle)
            {
                if (sle->getType() == ltCHECK)
                     result.push_back (sle);
            });
        return result;
    }

    static std::uint32_t
    ownerCount (test::jtx::Env const& env, test::jtx::Account const& account)
    {
        std::uint32_t ret {0};
        if (auto const sleAccount = env.le(account))
            ret = sleAccount->getFieldU32(sfOwnerCount);
        return ret;
    }

    void verifyDeliveredAmount (test::jtx::Env& env, STAmount const& amount)
    {
        std::string const txHash {
            env.tx()->getJson (JsonOptions::none)[jss::hash].asString()};

        env.close();
        Json::Value const meta = env.rpc ("tx", txHash)[jss::result][jss::meta];

        if (! BEAST_EXPECT(meta.isMember (sfDeliveredAmount.jsonName)))
            return;

        BEAST_EXPECT (meta[sfDeliveredAmount.jsonName] ==
            amount.getJson (JsonOptions::none));
        BEAST_EXPECT (meta[jss::delivered_amount] ==
            amount.getJson (JsonOptions::none));
    }

    void testEnabled()
    {
        testcase ("Enabled");

        using namespace test::jtx;
        Account const alice {"alice"};
        {
            Env env {*this, supported_amendments() - featureChecks};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice);

            uint256 const checkId {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)),
                ter(temDISABLED));
            env.close();

            env (check::cash (alice, checkId, XRP(100)),
                ter (temDISABLED));
            env.close();

            env (check::cancel (alice, checkId), ter (temDISABLED));
            env.close();
        }
        {
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice);

            uint256 const checkId1 {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)));
            env.close();

            env (check::cash (alice, checkId1, XRP(100)));
            env.close();

            uint256 const checkId2 {
                getCheckIndex (env.master, env.seq (env.master))};
            env (check::create (env.master, alice, XRP(100)));
            env.close();

            env (check::cancel (alice, checkId2));
            env.close();
        }
    }

    void testCreateValid()
    {
        testcase ("Create valid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        STAmount const startBalance {XRP(1000).value()};
        env.fund (startBalance, gw, alice, bob);

        auto writeTwoChecks =
            [&env, &USD, this] (Account const& from, Account const& to)
        {
            std::uint32_t const fromOwnerCount {ownerCount (env, from)};
            std::uint32_t const toOwnerCount   {ownerCount (env, to  )};

            std::size_t const fromCkCount {checksOnAccount (env, from).size()};
            std::size_t const toCkCount   {checksOnAccount (env, to  ).size()};

            env (check::create (from, to, XRP(2000)));
            env.close();

            env (check::create (from, to, USD(50)));
            env.close();

            BEAST_EXPECT (
                checksOnAccount (env, from).size() == fromCkCount + 2);
            BEAST_EXPECT (
                checksOnAccount (env, to  ).size() == toCkCount   + 2);

            env.require (owners (from, fromOwnerCount + 2));
            env.require (owners (to,
                to == from ? fromOwnerCount + 2 : toOwnerCount));
        };
        writeTwoChecks (alice,   bob);
        writeTwoChecks (gw,    alice);
        writeTwoChecks (alice,    gw);

        using namespace std::chrono_literals;
        std::size_t const aliceCount {checksOnAccount (env, alice).size()};
        std::size_t const bobCount   {checksOnAccount (env,   bob).size()};
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s));
        env.close();

        env (check::create (alice, bob, USD(50)), source_tag (2));
        env.close();
        env (check::create (alice, bob, USD(50)), dest_tag (3));
        env.close();
        env (check::create (alice, bob, USD(50)), invoice_id (uint256{4}));
        env.close();
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s),
            source_tag (12), dest_tag (13), invoice_id (uint256{4}));
        env.close();

        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 5);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 5);

        Account const alie {"alie", KeyType::ed25519};
        env (regkey (alice, alie));
        env.close();

        Account const bogie {"bogie", KeyType::secp256k1};
        Account const demon {"demon", KeyType::ed25519};
        env (signers (alice, 2, {{bogie, 1}, {demon, 1}}), sig (alie));
        env.close();

        env (check::create (alice, bob, USD(50)), sig (alie));
        env.close();
        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 6);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 6);

        std::uint64_t const baseFeeDrops {env.current()->fees().base};
        env (check::create (alice, bob, USD(50)),
            msig (bogie, demon), fee (3 * baseFeeDrops));
        env.close();
        BEAST_EXPECT (checksOnAccount (env, alice).size() == aliceCount + 7);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == bobCount   + 7);
    }

    void testCreateInvalid()
    {
        testcase ("Create invalid");

        using namespace test::jtx;

        Account const gw1 {"gateway1"};
        Account const gwF {"gatewayFrozen"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw1["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        STAmount const startBalance {XRP(1000).value()};
        env.fund (startBalance, gw1, gwF, alice, bob);

        env (check::create (alice, bob, USD(50)), fee (drops(-10)),
            ter (temBAD_FEE));
        env.close();

        env (check::create (alice, bob, USD(50)),
            txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
        env.close();

        env (check::create (alice, alice, XRP(10)), ter (temREDUNDANT));
        env.close();

        env (check::create (alice, bob, drops(-1)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, drops(0)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, drops(1)));
        env.close();

        env (check::create (alice, bob, USD(-1)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, USD(0)), ter (temBAD_AMOUNT));
        env.close();

        env (check::create (alice, bob, USD(1)));
        env.close();
        {
            IOU const BAD {gw1, badCurrency()};
            env (check::create (alice, bob, BAD(2)), ter (temBAD_CURRENCY));
            env.close();
        }

        env (check::create (alice, bob, USD(50)),
            expiration (NetClock::time_point{}), ter (temBAD_EXPIRATION));
        env.close();

        Account const bogie {"bogie"};
        env (check::create (alice, bogie, USD(50)), ter (tecNO_DST));
        env.close();

        env (fset (bob, asfRequireDest));
        env.close();

        env (check::create (alice, bob, USD(50)), ter (tecDST_TAG_NEEDED));
        env.close();

        env (check::create (alice, bob, USD(50)), dest_tag(11));
        env.close();

        env (fclear (bob, asfRequireDest));
        env.close();
        {
            IOU const USF {gwF["USF"]};
            env (fset(gwF, asfGlobalFreeze));
            env.close();

            env (check::create (alice, bob, USF(50)), ter (tecFROZEN));
            env.close();

            env (fclear(gwF, asfGlobalFreeze));
            env.close();

            env (check::create (alice, bob, USF(50)));
            env.close();
        }
        {
            env.trust (USD(1000), alice);
            env.trust (USD(1000), bob);
            env.close();
            env (pay (gw1, alice, USD(25)));
            env (pay (gw1, bob, USD(25)));
            env.close();

            env (trust(gw1, alice["USD"](0), tfSetFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (alice, bob, USD(1)), ter (tecPATH_DRY));
            env.close();
            env (check::create (bob, alice, USD(50)));
            env.close();
            env (pay (bob, alice, USD(1)));
            env.close();
            env (check::create (gw1, alice, USD(50)));
            env.close();
            env (pay (gw1, alice, USD(1)));
            env.close();

            env (trust(gw1, alice["USD"](0), tfClearFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)));
            env.close();
            env (check::create (bob, alice, USD(50)));
            env.close();
            env (check::create (gw1, alice, USD(50)));
            env.close();

            env (trust(alice, USD(0), tfSetFreeze));
            env.close();
            env (check::create (alice, bob, USD(50)));
            env.close();
            env (pay (alice, bob, USD(1)));
            env.close();
            env (check::create (bob, alice, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (bob, alice, USD(1)), ter (tecPATH_DRY));
            env.close();
            env (check::create (gw1, alice, USD(50)), ter (tecFROZEN));
            env.close();
            env (pay (gw1, alice, USD(1)), ter (tecPATH_DRY));
            env.close();

            env(trust(alice, USD(0), tfClearFreeze));
            env.close();
        }

        env (check::create (alice, bob, USD(50)),
            expiration (env.now()), ter (tecEXPIRED));
        env.close();

        using namespace std::chrono_literals;
        env (check::create (alice, bob, USD(50)), expiration (env.now() + 1s));
        env.close();

        Account const cheri {"cheri"};
        env.fund (
            env.current()->fees().accountReserve(1) - drops(1), cheri);

        env (check::create (cheri, bob, USD(50)),
            fee (drops (env.current()->fees().base)),
            ter (tecINSUFFICIENT_RESERVE));
        env.close();

        env (pay (bob, cheri, drops (env.current()->fees().base + 1)));
        env.close();

        env (check::create (cheri, bob, USD(50)));
        env.close();
    }

    void testCashXRP()
    {
        testcase ("Cash XRP");

        using namespace test::jtx;

        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        std::uint64_t const baseFeeDrops {env.current()->fees().base};
        STAmount const startBalance {XRP(300).value()};
        env.fund (startBalance, alice, bob);
        {
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();
            env.require (balance (alice, startBalance - drops (baseFeeDrops)));
            env.require (balance (bob,   startBalance));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

            env (check::cash (bob, chkId, XRP(10)));
            env.close();
            env.require (balance (alice,
                startBalance - XRP(10) - drops (baseFeeDrops)));
            env.require (balance (bob,
                startBalance + XRP(10) - drops (baseFeeDrops)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

            env (pay (env.master, alice, XRP(10) + drops (baseFeeDrops)));
            env (pay (bob, env.master,   XRP(10) - drops (baseFeeDrops * 2)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
        {
            STAmount const reserve {env.current()->fees().accountReserve (0)};
            STAmount const checkAmount {
                startBalance - reserve - drops (baseFeeDrops)};
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, checkAmount));
            env.close();

            env (check::cash (bob, chkId, checkAmount + drops(1)),
                ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (
                bob, chkId, check::DeliverMin (checkAmount + drops(1))),
                ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (checkAmount)));
            verifyDeliveredAmount (env, drops(checkAmount.mantissa()));
            env.require (balance (alice, reserve));
            env.require (balance (bob,
                startBalance + checkAmount - drops (baseFeeDrops * 3)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

            env (pay (env.master, alice, checkAmount + drops (baseFeeDrops)));
            env (pay (bob, env.master, checkAmount - drops (baseFeeDrops * 4)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
        {
            STAmount const reserve {env.current()->fees().accountReserve (0)};
            STAmount const checkAmount {
                startBalance - reserve - drops (baseFeeDrops - 1)};
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, checkAmount));
            env.close();

            env (check::cash (bob, chkId, checkAmount), ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (drops(1))));
            verifyDeliveredAmount (env, drops(checkAmount.mantissa() - 1));
            env.require (balance (alice, reserve));
            env.require (balance (bob,
                startBalance + checkAmount - drops ((baseFeeDrops * 2) + 1)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 0);
            BEAST_EXPECT (ownerCount (env, bob  ) == 0);

            env (pay (env.master, alice,
                checkAmount + drops (baseFeeDrops - 1)));
            env (pay (bob, env.master,
                checkAmount - drops ((baseFeeDrops * 3) + 1)));
            env.close();
            env.require (balance (alice, startBalance));
            env.require (balance (bob,   startBalance));
        }
    }

    void testCashIOU ()
    {
        testcase ("Cash IOU");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};
        {
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

            env (trust (alice, USD(20)));
            env.close();
            env (pay (gw, alice, USD(9.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

            env (pay (gw, alice, USD(0.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecNO_LINE));
            env.close();

            env (trust (bob, USD(9.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10)), ter (tecPATH_PARTIAL));
            env.close();

            env (trust (bob, USD(10.5)));
            env.close();
            env (check::cash (bob, chkId1, USD(10.5)), ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId1, USD(10)));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (check::cash (bob, chkId1, USD(10)), ter (tecNO_ENTRY));
            env.close();

            env (pay (bob, alice, USD(7)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);

            env (check::cash (bob, chkId2, USD(5)));
            env.close();
            env.require (balance (alice, USD(2)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            uint256 const chkId3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();
            uint256 const chkId4 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 2);

            env (check::cash (bob, chkId4, USD(2)));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (check::cash (bob, chkId3, USD(0)), ter (temBAD_AMOUNT));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (check::cancel (bob, chkId3));
            env.close();
            env.require (balance (alice, USD( 0)));
            env.require (balance (bob,   USD(10)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }
        {
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

            env (trust (alice, USD(20)));
            env (trust (bob, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

            uint256 const chkId9 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(9)));
            env.close();
            uint256 const chkId8 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(8)));
            env.close();
            uint256 const chkId7 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();
            uint256 const chkId6 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(6)));
            env.close();

            env (check::cash (bob, chkId9, check::DeliverMin (USD(9))),
                ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId9, check::DeliverMin (USD(7))));
            verifyDeliveredAmount (env, USD(8));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 3);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 3);
            BEAST_EXPECT (ownerCount (env, alice) == 4);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (pay (bob, alice, USD(7)));
            env.close();

            env (check::cash (bob, chkId7, check::DeliverMin (USD(7))));
            verifyDeliveredAmount (env, USD(7));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 2);
            BEAST_EXPECT (ownerCount (env, alice) == 3);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (pay (bob, alice, USD(8)));
            env.close();

            env (check::cash (bob, chkId6, check::DeliverMin (USD(4))));
            verifyDeliveredAmount (env, USD(6));
            env.require (balance (alice, USD(2)));
            env.require (balance (bob,   USD(6)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);

            env (check::cash (bob, chkId8, check::DeliverMin (USD(2))));
            verifyDeliveredAmount (env, USD(2));
            env.require (balance (alice, USD(0)));
            env.require (balance (bob,   USD(8)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }
        {
            Env env {*this};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);
            env (fset (gw, asfRequireAuth));
            env.close();
            env (trust (gw, alice["USD"](100)), txflags (tfSetfAuth));
            env (trust (alice, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(7)));
            env.close();

            env (check::cash (bob, chkId, USD(7)), ter (tecNO_LINE));
            env.close();

            env (trust (bob, USD(5)));
            env.close();

            env (check::cash (bob, chkId, USD(7)), ter (tecNO_AUTH));
            env.close();

            env (trust (gw, bob["USD"](1)), txflags (tfSetfAuth));
            env.close();

            env (check::cash (bob, chkId, USD(7)), ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (USD(4))));
            verifyDeliveredAmount (env, USD(5));
            env.require (balance (alice, USD(3)));
            env.require (balance (bob,   USD(5)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == 1);
        }

        FeatureBitset const allSupported {supported_amendments()};
        for (auto const features :
            {allSupported - featureMultiSignReserve,
                allSupported | featureMultiSignReserve})
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob);

            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(1)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(2)));
            env.close();

            env (trust (alice, USD(20)));
            env (trust (bob, USD(20)));
            env.close();
            env (pay (gw, alice, USD(8)));
            env.close();

            Account const bobby {"bobby", KeyType::secp256k1};
            env (regkey (bob, bobby));
            env.close();

            Account const bogie {"bogie", KeyType::secp256k1};
            Account const demon {"demon", KeyType::ed25519};
            env (signers (bob, 2, {{bogie, 1}, {demon, 1}}), sig (bobby));
            env.close();

            int const signersCount {features[featureMultiSignReserve] ? 1 : 4};
            BEAST_EXPECT (ownerCount (env, bob) == signersCount + 1);

            env (check::cash (bob, chkId1, (USD(1))), sig (bobby));
            env.close();
            env.require (balance (alice, USD(7)));
            env.require (balance (bob,   USD(1)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == 2);
            BEAST_EXPECT (ownerCount (env, bob  ) == signersCount + 1);

            std::uint64_t const baseFeeDrops {env.current()->fees().base};
            env (check::cash (bob, chkId2, (USD(2))),
                msig (bogie, demon), fee (3 * baseFeeDrops));
            env.close();
            env.require (balance (alice, USD(5)));
            env.require (balance (bob,   USD(3)));
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == 1);
            BEAST_EXPECT (ownerCount (env, bob  ) == signersCount + 1);
        }
    }

    void testCashXferFee()
    {
        testcase ("Cash with transfer fee");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob);

        env (trust (alice, USD(1000)));
        env (trust (bob, USD(1000)));
        env.close();
        env (pay (gw, alice, USD(1000)));
        env.close();

        env (rate (gw, 1.25));
        env.close();

        uint256 const chkId125 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(125)));
        env.close();

        uint256 const chkId120 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(120)));
        env.close();

        env (check::cash (bob, chkId125, USD(125)), ter (tecPATH_PARTIAL));
        env.close();
        env (check::cash (bob, chkId125, check::DeliverMin (USD(101))),
            ter (tecPATH_PARTIAL));
        env.close();

        env (check::cash (bob, chkId125, check::DeliverMin (USD(75))));
        verifyDeliveredAmount (env, USD(100));
        env.require (balance (alice, USD(1000 - 125)));
        env.require (balance (bob,   USD(   0 + 100)));
        BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 1);

        env (rate (gw, 1.2));
        env.close();

        env (check::cash (bob, chkId120, USD(50)));
        env.close();
        env.require (balance (alice, USD(1000 - 125 - 60)));
        env.require (balance (bob,   USD(   0 + 100 + 50)));
        BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
        BEAST_EXPECT (checksOnAccount (env, bob  ).size() == 0);
    }

    void testCashQuality ()
    {
        testcase ("Cash quality");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob);

        env (trust (alice, USD(1000)));
        env (trust (bob, USD(1000)));
        env.close();
        env (pay (gw, alice, USD(1000)));
        env.close();


        auto qIn =
            [] (double percent) { return qualityInPercent  (percent); };
        auto qOut =
            [] (double percent) { return qualityOutPercent (percent); };

        auto testNonIssuerQPay = [&env, &alice, &bob, &USD]
        (Account const& truster,
            IOU const& iou, auto const& inOrOut, double pct, double amount)
        {
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};
            STAmount const bobStart   {env.balance (bob,   USD.issue()).value()};

            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            env (pay (alice, bob, USD(amount)), sendmax (USD(10)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));
            env.require (balance (bob,   bobStart   + USD(10)));

            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        auto testNonIssuerQCheck = [&env, &alice, &bob, &USD]
        (Account const& truster,
            IOU const& iou, auto const& inOrOut, double pct, double amount)
        {
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};
            STAmount const bobStart   {env.balance (bob,   USD.issue()).value()};

            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            env (check::cash (bob, chkId, USD(amount)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));
            env.require (balance (bob,   bobStart   + USD(10)));

            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        testNonIssuerQPay   (alice, gw["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (alice, gw["USD"],  qIn,  50,     10);

        testNonIssuerQPay   (bob,   gw["USD"],  qIn,  50,      5);
        testNonIssuerQCheck (bob,   gw["USD"],  qIn,  50,      5);

        testNonIssuerQPay   (gw, alice["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (gw, alice["USD"],  qIn,  50,     10);

        testNonIssuerQPay   (gw,   bob["USD"],  qIn,  50,     10);
        testNonIssuerQCheck (gw,   bob["USD"],  qIn,  50,     10);

        testNonIssuerQPay   (alice, gw["USD"], qOut, 200,     10);
        testNonIssuerQCheck (alice, gw["USD"], qOut, 200,     10);

        testNonIssuerQPay   (bob,   gw["USD"], qOut, 200,     10);
        testNonIssuerQCheck (bob,   gw["USD"], qOut, 200,     10);

        testNonIssuerQPay   (gw, alice["USD"], qOut, 200,     10);
        testNonIssuerQCheck (gw, alice["USD"], qOut, 200,     10);

        testNonIssuerQPay   (gw,   bob["USD"], qOut, 200,     10);
        testNonIssuerQCheck (gw,   bob["USD"], qOut, 200,     10);


        auto testIssuerQPay = [&env, &gw, &alice, &USD]
        (Account const& truster, IOU const& iou,
            auto const& inOrOut, double pct,
            double amt1, double max1, double amt2, double max2)
        {
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};

            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            env (pay (alice, gw, USD(amt1)), sendmax (USD(max1)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));

            env (pay (gw, alice, USD(amt2)), sendmax (USD(max2)));
            env.close();
            env.require (balance (alice, aliceStart));

            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        auto testIssuerQCheck = [&env, &gw, &alice, &USD]
        (Account const& truster, IOU const& iou,
            auto const& inOrOut, double pct,
            double amt1, double max1, double amt2, double max2)
        {
            STAmount const aliceStart {env.balance (alice, USD.issue()).value()};

            env (trust (truster, iou(1000)), inOrOut (pct));
            env.close();

            uint256 const chkAliceId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, gw, USD(max1)));
            env.close();

            env (check::cash (gw, chkAliceId, USD(amt1)));
            env.close();
            env.require (balance (alice, aliceStart - USD(10)));

            uint256 const chkGwId {getCheckIndex (gw, env.seq (gw))};
            env (check::create (gw, alice, USD(max2)));
            env.close();

            env (check::cash (alice, chkGwId, USD(amt2)));
            env.close();
            env.require (balance (alice, aliceStart));

            env (trust (truster, iou(1000)), inOrOut (0));
            env.close();
        };

        testIssuerQPay   (alice, gw["USD"],  qIn,  50,   10,  10,   5,  10);
        testIssuerQCheck (alice, gw["USD"],  qIn,  50,   10,  10,   5,  10);

        testIssuerQPay   (gw, alice["USD"],  qIn,  50,   10,  10,  10,  10);
        testIssuerQCheck (gw, alice["USD"],  qIn,  50,   10,  10,  10,  10);

        testIssuerQPay   (alice, gw["USD"], qOut, 200,   10,  10,  10,  10);
        testIssuerQCheck (alice, gw["USD"], qOut, 200,   10,  10,  10,  10);

        testIssuerQPay   (gw, alice["USD"], qOut, 200,   10,  10,  10,  10);
        testIssuerQCheck (gw, alice["USD"], qOut, 200,   10,  10,  10,  10);
    }

    void testCashInvalid()
    {
        testcase ("Cash invalid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const zoe {"zoe"};
        IOU const USD {gw["USD"]};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), gw, alice, bob, zoe);

        env (trust (alice, USD(20)));
        env.close();
        env (pay (gw, alice, USD(20)));
        env.close();

        {
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(20)));
            env.close();

            env (check::cash (bob, chkId, USD(20)), ter (tecNO_LINE));
            env.close();
        }

        env (trust (bob, USD(20)));
        env.close();

        {
            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::cash (bob, chkId, USD(20)), ter (tecNO_ENTRY));
            env.close();
        }

        uint256 const chkIdU {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(20)));
        env.close();

        uint256 const chkIdX {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, XRP(10)));
        env.close();

        using namespace std::chrono_literals;
        uint256 const chkIdExp {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, XRP(10)), expiration (env.now() + 1s));
        env.close();

        uint256 const chkIdFroz1 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(1)));
        env.close();

        uint256 const chkIdFroz2 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(2)));
        env.close();

        uint256 const chkIdFroz3 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(3)));
        env.close();

        uint256 const chkIdFroz4 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(4)));
        env.close();

        uint256 const chkIdNoDest1 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(1)));
        env.close();

        uint256 const chkIdHasDest2 {getCheckIndex (alice, env.seq (alice))};
        env (check::create (alice, bob, USD(2)), dest_tag (7));
        env.close();

        auto failingCases = [&env, &gw, &alice, &bob] (
            uint256 const& chkId, STAmount const& amount)
        {
            env (check::cash (bob, chkId, amount), fee (drops(-10)),
                ter (temBAD_FEE));
            env.close();

            env (check::cash (bob, chkId, amount),
                txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
            env.close();

            {
                Json::Value tx {check::cash (bob, chkId, amount)};
                tx.removeMember (sfAmount.jsonName);
                env (tx, ter (temMALFORMED));
                env.close();
            }
            {
                Json::Value tx {check::cash (bob, chkId, amount)};
                tx[sfDeliverMin.jsonName]  = amount.getJson(JsonOptions::none);
                env (tx, ter (temMALFORMED));
                env.close();
            }

            {
                STAmount neg {amount};
                neg.negate();
                env (check::cash (bob, chkId, neg), ter (temBAD_AMOUNT));
                env.close();
                env (check::cash (bob, chkId, amount.zeroed()),
                    ter (temBAD_AMOUNT));
                env.close();
            }

            if (! amount.native()) {
                Issue const badIssue {badCurrency(), amount.getIssuer()};
                STAmount badAmount {amount};
                badAmount.setIssue (Issue {badCurrency(), amount.getIssuer()});
                env (check::cash (bob, chkId, badAmount),
                    ter (temBAD_CURRENCY));
                env.close();
            }

            env (check::cash (alice, chkId, amount), ter (tecNO_PERMISSION));
            env.close();
            env (check::cash (gw, chkId, amount), ter (tecNO_PERMISSION));
            env.close();

            {
                IOU const wrongCurrency {gw["EUR"]};
                STAmount badAmount {amount};
                badAmount.setIssue (wrongCurrency.issue());
                env (check::cash (bob, chkId, badAmount),
                    ter (temMALFORMED));
                env.close();
            }

            {
                IOU const wrongIssuer {alice["USD"]};
                STAmount badAmount {amount};
                badAmount.setIssue (wrongIssuer.issue());
                env (check::cash (bob, chkId, badAmount),
                    ter (temMALFORMED));
                env.close();
            }

            env (check::cash (bob, chkId, amount + amount),
                ter (tecPATH_PARTIAL));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (amount + amount)),
                ter (tecPATH_PARTIAL));
            env.close();
        };

        failingCases (chkIdX, XRP(10));
        failingCases (chkIdU, USD(20));

        env (check::cash (bob, chkIdU, USD(20)));
        env.close();
        env (check::cash (bob, chkIdX, check::DeliverMin (XRP(10))));
        verifyDeliveredAmount (env, XRP(10));

        env (check::cash (bob, chkIdExp, XRP(10)), ter (tecEXPIRED));
        env.close();

        env (check::cancel (zoe, chkIdExp));
        env.close();

        {
            env (pay (bob, alice, USD(20)));
            env.close();
            env.require (balance (alice, USD(20)));
            env.require (balance (bob,   USD( 0)));

            env (fset (gw, asfGlobalFreeze));
            env.close();

            env (check::cash (bob, chkIdFroz1, USD(1)), ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (bob, chkIdFroz1, check::DeliverMin (USD(0.5))),
                ter (tecPATH_PARTIAL));
            env.close();

            env (fclear (gw, asfGlobalFreeze));
            env.close();

            env (check::cash (bob, chkIdFroz1, USD(1)));
            env.close();
            env.require (balance (alice, USD(19)));
            env.require (balance (bob,   USD( 1)));

            env (trust(gw, alice["USD"](0), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz2, USD(2)), ter (tecPATH_PARTIAL));
            env.close();
            env (check::cash (bob, chkIdFroz2, check::DeliverMin (USD(1))),
                ter (tecPATH_PARTIAL));
            env.close();

            env (trust(gw, alice["USD"](0), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz2, USD(2)));
            env.close();
            env.require (balance (alice, USD(17)));
            env.require (balance (bob,   USD( 3)));

            env (trust(gw, bob["USD"](0), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz3, USD(3)), ter (tecFROZEN));
            env.close();
            env (check::cash (bob, chkIdFroz3, check::DeliverMin (USD(1))),
                ter (tecFROZEN));
            env.close();

            env (trust(gw, bob["USD"](0), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz3, check::DeliverMin (USD(1))));
            verifyDeliveredAmount (env, USD(3));
            env.require (balance (alice, USD(14)));
            env.require (balance (bob,   USD( 6)));

            env (trust (bob, USD(20), tfSetFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz4, USD(4)), ter (terNO_LINE));
            env.close();
            env (check::cash (bob, chkIdFroz4, check::DeliverMin (USD(1))),
                ter (terNO_LINE));
            env.close();

            env (trust (bob, USD(20), tfClearFreeze));
            env.close();
            env (check::cash (bob, chkIdFroz4, USD(4)));
            env.close();
            env.require (balance (alice, USD(10)));
            env.require (balance (bob,   USD(10)));
        }
        {
            env (fset (bob, asfRequireDest));
            env.close();
            env (check::cash (bob, chkIdNoDest1, USD(1)),
                ter (tecDST_TAG_NEEDED));
            env.close();
            env (check::cash (bob, chkIdNoDest1, check::DeliverMin (USD(0.5))),
                ter (tecDST_TAG_NEEDED));
            env.close();

            env (check::cash (bob, chkIdHasDest2, USD(2)));
            env.close();
            env.require (balance (alice, USD( 8)));
            env.require (balance (bob,   USD(12)));

            env (fclear (bob, asfRequireDest));
            env.close();
            env (check::cash (bob, chkIdNoDest1, USD(1)));
            env.close();
            env.require (balance (alice, USD( 7)));
            env.require (balance (bob,   USD(13)));
        }
    }

    void testCancelValid()
    {
        testcase ("Cancel valid");

        using namespace test::jtx;

        Account const gw {"gateway"};
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const zoe {"zoe"};
        IOU const USD {gw["USD"]};

        FeatureBitset const allSupported {supported_amendments()};
        for (auto const features :
            {allSupported - featureMultiSignReserve,
                allSupported | featureMultiSignReserve})
        {
            Env env {*this, features};

            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), gw, alice, bob, zoe);

            uint256 const chkId1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            uint256 const chkId2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();

            uint256 const chkId3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            using namespace std::chrono_literals;
            uint256 const chkIdNotExp1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+600s));
            env.close();

            uint256 const chkIdNotExp2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+600s));
            env.close();

            uint256 const chkIdNotExp3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+600s));
            env.close();

            uint256 const chkIdExp1 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+1s));
            env.close();

            uint256 const chkIdExp2 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)), expiration (env.now()+1s));
            env.close();

            uint256 const chkIdExp3 {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)), expiration (env.now()+1s));
            env.close();

            uint256 const chkIdReg {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, USD(10)));
            env.close();

            uint256 const chkIdMSig {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(10)));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 11);
            BEAST_EXPECT (ownerCount (env, alice) == 11);

            env (check::cancel (alice, chkId1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 10);
            BEAST_EXPECT (ownerCount (env, alice) == 10);

            env (check::cancel (bob, chkId2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 9);
            BEAST_EXPECT (ownerCount (env, alice) == 9);

            env (check::cancel (zoe, chkId3), ter (tecNO_PERMISSION));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 9);
            BEAST_EXPECT (ownerCount (env, alice) == 9);

            env (check::cancel (alice, chkIdNotExp1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 8);
            BEAST_EXPECT (ownerCount (env, alice) == 8);

            env (check::cancel (bob, chkIdNotExp2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 7);
            BEAST_EXPECT (ownerCount (env, alice) == 7);

            env (check::cancel (zoe, chkIdNotExp3), ter (tecNO_PERMISSION));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 7);
            BEAST_EXPECT (ownerCount (env, alice) == 7);

            env (check::cancel (alice, chkIdExp1));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 6);
            BEAST_EXPECT (ownerCount (env, alice) == 6);

            env (check::cancel (bob, chkIdExp2));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 5);
            BEAST_EXPECT (ownerCount (env, alice) == 5);

            env (check::cancel (zoe, chkIdExp3));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 4);
            BEAST_EXPECT (ownerCount (env, alice) == 4);

            Account const alie {"alie", KeyType::ed25519};
            env (regkey (alice, alie));
            env.close();

            Account const bogie {"bogie", KeyType::secp256k1};
            Account const demon {"demon", KeyType::ed25519};
            env (signers (alice, 2, {{bogie, 1}, {demon, 1}}), sig (alie));
            env.close();

            int const signersCount {features[featureMultiSignReserve] ? 1 : 4};

            env (check::cancel (alice, chkIdReg), sig (alie));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 3);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 3);

            std::uint64_t const baseFeeDrops {env.current()->fees().base};
            env (check::cancel (alice, chkIdMSig),
                msig (bogie, demon), fee (3 * baseFeeDrops));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 2);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 2);

            env (check::cancel (alice, chkId3), sig (alice));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 1);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 1);

            env (check::cancel (bob, chkIdNotExp3));
            env.close();
            BEAST_EXPECT (checksOnAccount (env, alice).size() == 0);
            BEAST_EXPECT (ownerCount (env, alice) == signersCount + 0);
        }
    }

    void testCancelInvalid()
    {
        testcase ("Cancel invalid");

        using namespace test::jtx;

        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env {*this};
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000), alice, bob);

        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            fee (drops(-10)), ter (temBAD_FEE));
        env.close();

        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            txflags (tfImmediateOrCancel), ter (temINVALID_FLAG));
        env.close();

        env (check::cancel (bob, getCheckIndex (alice, env.seq (alice))),
            ter (tecNO_ENTRY));
        env.close();
    }

    void testFix1623Enable ()
    {
        testcase ("Fix1623 enable");

        using namespace test::jtx;

        auto testEnable = [this] (FeatureBitset const& features, bool hasFields)
        {
            Account const alice {"alice"};
            Account const bob {"bob"};

            Env env {*this, features};
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP(1000), alice, bob);
            env.close();

            uint256 const chkId {getCheckIndex (alice, env.seq (alice))};
            env (check::create (alice, bob, XRP(200)));
            env.close();

            env (check::cash (bob, chkId, check::DeliverMin (XRP(100))));

            std::string const txHash {
                env.tx()->getJson (JsonOptions::none)[jss::hash].asString()};

            env.close();
            Json::Value const meta =
                env.rpc ("tx", txHash)[jss::result][jss::meta];

            BEAST_EXPECT(meta.isMember (
                sfDeliveredAmount.jsonName) == hasFields);
            BEAST_EXPECT(meta.isMember (jss::delivered_amount) == hasFields);
        };

        testEnable (supported_amendments() - fix1623, false);
        testEnable (supported_amendments(),           true);
    }

public:
    void run () override
    {
        testEnabled();
        testCreateValid();
        testCreateInvalid();
        testCashXRP();
        testCashIOU();
        testCashXferFee();
        testCashQuality();
        testCashInvalid();
        testCancelValid();
        testCancelInvalid();
        testFix1623Enable();
    }
};

BEAST_DEFINE_TESTSUITE (Check, tx, ripple);

}  

























