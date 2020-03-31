

#ifndef RIPPLE_LEDGER_OPENVIEW_H_INCLUDED
#define RIPPLE_LEDGER_OPENVIEW_H_INCLUDED

#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/detail/RawStateTable.h>
#include <ripple/basics/qalloc.h>
#include <ripple/protocol/XRPAmount.h>
#include <functional>
#include <utility>

namespace ripple {


struct open_ledger_t
{
    explicit open_ledger_t() = default;
};

extern open_ledger_t const open_ledger;



class OpenView final
    : public ReadView
    , public TxsRawView
{
private:
    class txs_iter_impl;

    using txs_map = std::map<key_type,
        std::pair<std::shared_ptr<Serializer const>,
        std::shared_ptr<Serializer const>>,
        std::less<key_type>, qalloc_type<std::pair<key_type const,
        std::pair<std::shared_ptr<Serializer const>,
        std::shared_ptr<Serializer const>>>, false>>;

    Rules rules_;
    txs_map txs_;
    LedgerInfo info_;
    ReadView const* base_;
    detail::RawStateTable items_;
    std::shared_ptr<void const> hold_;
    bool open_ = true;

public:
    OpenView() = delete;
    OpenView& operator= (OpenView&&) = delete;
    OpenView& operator= (OpenView const&) = delete;

    OpenView (OpenView&&) = default;

    
    OpenView (OpenView const&) = default;

    
    
    OpenView (open_ledger_t,
        ReadView const* base, Rules const& rules,
            std::shared_ptr<void const> hold = nullptr);

    OpenView (open_ledger_t, Rules const& rules,
            std::shared_ptr<ReadView const> const& base)
        : OpenView (open_ledger, &*base, rules, base)
    {
    }
    

    
    OpenView (ReadView const* base,
        std::shared_ptr<void const> hold = nullptr);

    
    bool
    open() const override
    {
        return open_;
    }

    
    std::size_t
    txCount() const;

    
    void
    apply (TxsRawView& to) const;


    LedgerInfo const&
    info() const override;

    Fees const&
    fees() const override;

    Rules const&
    rules() const override;

    bool
    exists (Keylet const& k) const override;

    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (key_type const& key) const override;

    tx_type
    txRead (key_type const& key) const override;


    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyXRP(
        XRPAmount const& fee) override;


    void
    rawTxInsert (key_type const& key,
        std::shared_ptr<Serializer const>
            const& txn, std::shared_ptr<
                Serializer const>
                    const& metaData) override;
};

} 

#endif








