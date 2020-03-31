

#ifndef RIPPLE_APP_LEDGER_OPENLEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_OPENLEDGER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/ledger/CachedSLEs.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/core/Config.h>
#include <ripple/beast/utility/Journal.h>
#include <cassert>
#include <mutex>

namespace ripple {

#define LEDGER_TOTAL_PASSES 3

#define LEDGER_RETRY_PASSES 1

using OrderedTxs = CanonicalTXSet;



class OpenLedger
{
private:
    beast::Journal j_;
    CachedSLEs& cache_;
    std::mutex mutable modify_mutex_;
    std::mutex mutable current_mutex_;
    std::shared_ptr<OpenView const> current_;

public:
    
    using modify_type = std::function<
        bool(OpenView&, beast::Journal)>;

    OpenLedger() = delete;
    OpenLedger (OpenLedger const&) = delete;
    OpenLedger& operator= (OpenLedger const&) = delete;

    
    explicit
    OpenLedger(std::shared_ptr<
        Ledger const> const& ledger,
            CachedSLEs& cache,
                beast::Journal journal);

    
    bool
    empty() const;

    
    std::shared_ptr<OpenView const>
    current() const;

    
    bool
    modify (modify_type const& f);

    
    void
    accept (Application& app, Rules const& rules,
        std::shared_ptr<Ledger const> const& ledger,
            OrderedTxs const& locals, bool retriesFirst,
                OrderedTxs& retries, ApplyFlags flags,
                    std::string const& suffix = "",
                        modify_type const& f = {});

private:
    
    template <class FwdRange>
    static
    void
    apply (Application& app, OpenView& view,
        ReadView const& check, FwdRange const& txs,
            OrderedTxs& retries, ApplyFlags flags,
                std::map<uint256, bool>& shouldRecover,
                    beast::Journal j);

    enum Result
    {
        success,
        failure,
        retry
    };

    std::shared_ptr<OpenView>
    create (Rules const& rules,
        std::shared_ptr<Ledger const> const& ledger);

    static
    Result
    apply_one (Application& app, OpenView& view,
        std::shared_ptr< STTx const> const& tx,
            bool retry, ApplyFlags flags,
                bool shouldRecover, beast::Journal j);
};


template <class FwdRange>
void
OpenLedger::apply (Application& app, OpenView& view,
    ReadView const& check, FwdRange const& txs,
        OrderedTxs& retries, ApplyFlags flags,
            std::map<uint256, bool>& shouldRecover,
                beast::Journal j)
{
    for (auto iter = txs.begin();
        iter != txs.end(); ++iter)
    {
        try
        {
            auto const tx = *iter;
            auto const txId = tx->getTransactionID();
            if (check.txExists(txId))
                continue;
            auto const result = apply_one(app, view,
                tx, true, flags, shouldRecover[txId], j);
            if (result == Result::retry)
                retries.insert(tx);
        }
        catch(std::exception const&)
        {
            JLOG(j.error()) <<
                "Caught exception";
        }
    }
    bool retry = true;
    for (int pass = 0;
        pass < LEDGER_TOTAL_PASSES;
            ++pass)
    {
        int changes = 0;
        auto iter = retries.begin();
        while (iter != retries.end())
        {
            switch (apply_one(app, view,
                iter->second, retry, flags,
                    shouldRecover[iter->second->getTransactionID()], j))
            {
            case Result::success:
                ++changes;
            case Result::failure:
                iter = retries.erase (iter);
                break;
            case Result::retry:
                ++iter;
            }
        }
        if (! changes && ! retry)
            return;
        if (! changes || (pass >= LEDGER_RETRY_PASSES))
            retry = false;
    }

    assert (retries.empty() || ! retry);
}



std::string
debugTxstr (std::shared_ptr<STTx const> const& tx);

std::string
debugTostr (OrderedTxs const& set);

std::string
debugTostr (SHAMap const& set);

std::string
debugTostr (std::shared_ptr<ReadView const> const& view);

} 

#endif








