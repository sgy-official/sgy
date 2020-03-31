

#ifndef RIPPLE_TEST_JTX_ENV_H_INCLUDED
#define RIPPLE_TEST_JTX_ENV_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <test/jtx/envconfig.h>
#include <test/jtx/JTx.h>
#include <test/jtx/require.h>
#include <test/jtx/tags.h>
#include <test/jtx/AbstractClient.h>
#include <test/jtx/ManualTimeKeeper.h>
#include <test/unit_test/SuiteJournal.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_value.h>
#include <ripple/json/to_string.h>
#include <ripple/ledger/CachedSLEs.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <functional>
#include <string>
#include <tuple>
#include <ripple/beast/cxx17/type_traits.h> 
#include <utility>
#include <unordered_map>
#include <vector>


namespace ripple {
namespace test {
namespace jtx {


template <class... Args>
std::array<Account, 1 + sizeof...(Args)>
noripple (Account const& account, Args const&... args)
{
    return {{account, args...}};
}

inline
FeatureBitset
supported_amendments()
{
    static const FeatureBitset ids = []{
        auto const& sa = ripple::detail::supportedAmendments();
        std::vector<uint256> feats;
        feats.reserve(sa.size());
        for (auto const& s : sa)
        {
            if (auto const f = getRegisteredFeature(s))
                feats.push_back(*f);
            else
                Throw<std::runtime_error> ("Unknown feature: " + s + "  in supportedAmendments.");
        }
        return FeatureBitset(feats);
    }();
    return ids;
}


class SuiteLogs : public Logs
{
    beast::unit_test::suite& suite_;

public:
    explicit
    SuiteLogs(beast::unit_test::suite& suite)
        : Logs (beast::severities::kError)
        , suite_(suite)
    {
    }

    ~SuiteLogs() override = default;

    std::unique_ptr<beast::Journal::Sink>
    makeSink(std::string const& partition,
        beast::severities::Severity threshold) override
    {
        return std::make_unique<SuiteJournalSink>(
            partition, threshold, suite_);
    }
};



class Env
{
public:
    beast::unit_test::suite& test;

    Account const& master = Account::master;

private:
    struct AppBundle
    {
        Application* app;
        std::unique_ptr<Application> owned;
        ManualTimeKeeper* timeKeeper;
        std::thread thread;
        std::unique_ptr<AbstractClient> client;

        AppBundle (beast::unit_test::suite& suite,
            std::unique_ptr<Config> config,
            std::unique_ptr<Logs> logs);
        ~AppBundle();
    };

    AppBundle bundle_;

public:
    beast::Journal const journal;

    Env() = delete;
    Env& operator= (Env const&) = delete;
    Env (Env const&) = delete;

    
    Env (beast::unit_test::suite& suite_,
            std::unique_ptr<Config> config,
            FeatureBitset features,
            std::unique_ptr<Logs> logs = nullptr)
        : test (suite_)
        , bundle_ (
            suite_,
            std::move(config),
            logs ? std::move(logs) : std::make_unique<SuiteLogs>(suite_))
        , journal {bundle_.app->journal ("Env")}
    {
        memoize(Account::master);
        Pathfinder::initPathTable();
        foreachFeature(
            features, [& appFeats = app().config().features](uint256 const& f) {
                appFeats.insert(f);
            });
    }

    
    Env (beast::unit_test::suite& suite_,
            FeatureBitset features)
        : Env(suite_, envconfig(), features)
    {
    }

    
    Env (beast::unit_test::suite& suite_,
        std::unique_ptr<Config> config,
        std::unique_ptr<Logs> logs = nullptr)
        : Env(suite_, std::move(config),
            supported_amendments(), std::move(logs))
    {
    }

    
    Env (beast::unit_test::suite& suite_)
        : Env(suite_, envconfig())
    {
    }

    virtual ~Env() = default;

    Application&
    app()
    {
        return *bundle_.app;
    }

    Application const&
    app() const
    {
        return *bundle_.app;
    }

    ManualTimeKeeper&
    timeKeeper()
    {
        return *bundle_.timeKeeper;
    }

    
    NetClock::time_point
    now()
    {
        return timeKeeper().now();
    }

    
    AbstractClient&
    client()
    {
        return *bundle_.client;
    }

    
    template<class... Args>
    Json::Value
    rpc(std::unordered_map<std::string, std::string> const& headers,
        std::string const& cmd,
        Args&&... args);

    template<class... Args>
    Json::Value
    rpc(std::string const& cmd, Args&&... args);

    
    std::shared_ptr<OpenView const>
    current() const
    {
        return app().openLedger().current();
    }

    
    std::shared_ptr<ReadView const>
    closed();

    
    void
    close (NetClock::time_point closeTime,
        boost::optional<std::chrono::milliseconds> consensusDelay = boost::none);

    
    template <class Rep, class Period>
    void
    close (std::chrono::duration<
        Rep, Period> const& elapsed)
    {
        close (now() + elapsed);
    }

    
    void
    close()
    {
        close (std::chrono::seconds(5));
    }

    
    void
    trace (int howMany = -1)
    {
        trace_ = howMany;
    }

    
    void
    notrace ()
    {
        trace_ = 0;
    }

    
    void
    disable_sigs()
    {
        app().checkSigs(false);
    }

    
    void
    memoize (Account const& account);

    
    
    Account const&
    lookup (AccountID const& id) const;

    Account const&
    lookup (std::string const& base58ID) const;
    

    
    PrettyAmount
    balance (Account const& account) const;

    
    std::uint32_t
    seq (Account const& account) const;

    
    PrettyAmount
    balance (Account const& account,
        Issue const& issue) const;

    
    std::shared_ptr<SLE const>
    le (Account const& account) const;

    
    std::shared_ptr<SLE const>
    le (Keylet const& k) const;

    
    template <class JsonValue,
        class... FN>
    JTx
    jt (JsonValue&& jv, FN const&... fN)
    {
        JTx jt(std::forward<JsonValue>(jv));
        invoke(jt, fN...);
        autofill(jt);
        jt.stx = st(jt);
        return jt;
    }

    
    template <class JsonValue,
        class... FN>
    Json::Value
    json (JsonValue&&jv, FN const&... fN)
    {
        auto tj = jt(
            std::forward<JsonValue>(jv),
                fN...);
        return std::move(tj.jv);
    }

    
    template <class... Args>
    void
    require (Args const&... args)
    {
        jtx::required(args...)(*this);
    }

    
    static
    std::pair<TER, bool>
    parseResult(Json::Value const& jr);

    
    virtual
    void
    submit (JTx const& jt);

    
    void
    sign_and_submit(JTx const& jt, Json::Value params = Json::nullValue);

    
    void
    postconditions(JTx const& jt, TER ter, bool didApply);

    
    
    template <class JsonValue, class... FN>
    void
    apply (JsonValue&& jv, FN const&... fN)
    {
        submit(jt(std::forward<
            JsonValue>(jv), fN...));
    }

    template <class JsonValue,
        class... FN>
    void
    operator()(JsonValue&& jv, FN const&... fN)
    {
        apply(std::forward<
            JsonValue>(jv), fN...);
    }
    

    
    TER
    ter() const
    {
        return ter_;
    }

    
    std::shared_ptr<STObject const>
    meta();

    
    std::shared_ptr<STTx const>
    tx() const;

    void
    enableFeature(uint256 const feature);

private:
    void
    fund (bool setDefaultRipple,
        STAmount const& amount,
            Account const& account);

    inline
    void
    fund (STAmount const&)
    {
    }

    void
    fund_arg (STAmount const& amount,
        Account const& account)
    {
        fund (true, amount, account);
    }

    template <std::size_t N>
    void
    fund_arg (STAmount const& amount,
        std::array<Account, N> const& list)
    {
        for (auto const& account : list)
            fund (false, amount, account);
    }
public:

    
    template<class Arg, class... Args>
    void
    fund (STAmount const& amount,
        Arg const& arg, Args const&... args)
    {
        fund_arg (amount, arg);
        fund (amount, args...);
    }

    
    
    void
    trust (STAmount const& amount,
        Account const& account);

    template<class... Accounts>
    void
    trust (STAmount const& amount, Account const& to0,
        Account const& to1, Accounts const&... toN)
    {
        trust(amount, to0);
        trust(amount, to1, toN...);
    }
    

protected:
    int trace_ = 0;
    TestStopwatch stopwatch_;
    uint256 txid_;
    TER ter_ = tesSUCCESS;

    Json::Value
    do_rpc(std::vector<std::string> const& args,
        std::unordered_map<std::string, std::string> const& headers = {});

    void
    autofill_sig (JTx& jt);

    virtual
    void
    autofill (JTx& jt);

    
    std::shared_ptr<STTx const>
    st (JTx const& jt);

    template <class... FN>
    void
    invoke (STTx& stx, FN const&... fN)
    {
        (void)std::array<int, sizeof...(fN)>{{((fN(*this, stx)), 0)...}};
    }

    template <class... FN>
    void
    invoke (JTx& jt, FN const&... fN)
    {
        (void)std::array<int, sizeof...(fN)>{{((fN(*this, jt)), 0)...}};
    }

    std::unordered_map<
        AccountID, Account> map_;
};

template<class... Args>
Json::Value
Env::rpc(std::unordered_map<std::string, std::string> const& headers,
    std::string const& cmd,
    Args&&... args)
{
    return do_rpc(std::vector<std::string>{cmd, std::forward<Args>(args)...},
        headers);
}

template<class... Args>
Json::Value
Env::rpc(std::string const& cmd, Args&&... args)
{
    return rpc(std::unordered_map<std::string, std::string>(), cmd,
        std::forward<Args>(args)...);
}

} 
} 
} 

#endif








