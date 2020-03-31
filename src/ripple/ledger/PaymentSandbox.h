

#ifndef RIPPLE_LEDGER_PAYMENTSANDBOX_H_INCLUDED
#define RIPPLE_LEDGER_PAYMENTSANDBOX_H_INCLUDED

#include <ripple/ledger/RawView.h>
#include <ripple/ledger/Sandbox.h>
#include <ripple/ledger/detail/ApplyViewBase.h>
#include <ripple/protocol/AccountID.h>
#include <map>
#include <utility>

namespace ripple {

namespace detail {

class DeferredCredits
{
public:
    struct Adjustment
    {
        Adjustment (STAmount const& d, STAmount const& c, STAmount const& b)
            : debits (d), credits (c), origBalance (b) {}
        STAmount debits;
        STAmount credits;
        STAmount origBalance;
    };

    boost::optional<Adjustment> adjustments (
        AccountID const& main,
        AccountID const& other,
        Currency const& currency) const;

    void credit (AccountID const& sender,
        AccountID const& receiver,
        STAmount const& amount,
        STAmount const& preCreditSenderBalance);

    void ownerCount (AccountID const& id,
        std::uint32_t cur,
            std::uint32_t next);

    boost::optional<std::uint32_t>
    ownerCount (AccountID const& id) const;

    void apply (DeferredCredits& to);
private:
    using Key = std::tuple<
        AccountID, AccountID, Currency>;
    struct Value
    {
        explicit Value() = default;

        STAmount lowAcctCredits;
        STAmount highAcctCredits;
        STAmount lowAcctOrigBalance;
    };

    static
    Key
    makeKey (AccountID const& a1,
        AccountID const& a2,
            Currency const& c);

    std::map<Key, Value> credits_;
    std::map<AccountID, std::uint32_t> ownerCounts_;
};

} 



class PaymentSandbox final
    : public detail::ApplyViewBase
{
public:
    PaymentSandbox() = delete;
    PaymentSandbox (PaymentSandbox const&) = delete;
    PaymentSandbox& operator= (PaymentSandbox&&) = delete;
    PaymentSandbox& operator= (PaymentSandbox const&) = delete;

    PaymentSandbox (PaymentSandbox&&) = default;

    PaymentSandbox (ReadView const* base, ApplyFlags flags)
        : ApplyViewBase (base, flags)
    {
    }

    PaymentSandbox (ApplyView const* base)
        : ApplyViewBase (base, base->flags())
    {
    }

    
    
    explicit
    PaymentSandbox (PaymentSandbox const* base)
        : ApplyViewBase(base, base->flags())
        , ps_ (base)
    {
    }

    explicit
    PaymentSandbox (PaymentSandbox* base)
        : ApplyViewBase(base, base->flags())
        , ps_ (base)
    {
    }
    

    STAmount
    balanceHook (AccountID const& account,
        AccountID const& issuer,
            STAmount const& amount) const override;

    void
    creditHook (AccountID const& from,
        AccountID const& to,
            STAmount const& amount,
                STAmount const& preCreditBalance) override;

    void
    adjustOwnerCountHook (AccountID const& account,
        std::uint32_t cur,
            std::uint32_t next) override;

    std::uint32_t
    ownerCountHook (AccountID const& account,
        std::uint32_t count) const override;

    
    
    void apply (RawView& to);

    void
    apply (PaymentSandbox& to);
    

    std::map<std::tuple<AccountID, AccountID, Currency>, STAmount>
    balanceChanges (ReadView const& view) const;

    XRPAmount xrpDestroyed () const;

private:
    detail::DeferredCredits tab_;
    PaymentSandbox const* ps_ = nullptr;
};

}  

#endif








