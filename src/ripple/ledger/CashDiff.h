

#ifndef RIPPLE_LEDGER_CASHDIFF_H_INCLUDED
#define RIPPLE_LEDGER_CASHDIFF_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/STAmount.h>
#include <memory>                       

namespace ripple {

class ReadView;

namespace detail {

class ApplyStateTable;

}

enum class CashFilter : std::uint8_t
{
    none = 0x0,
    treatZeroOfferAsDeletion = 0x1
};
inline CashFilter operator| (CashFilter lhs, CashFilter rhs)
{
    using ul_t = std::underlying_type<CashFilter>::type;
    return static_cast<CashFilter>(
        safe_cast<ul_t>(lhs) | safe_cast<ul_t>(rhs));
}
inline CashFilter operator& (CashFilter lhs, CashFilter rhs)
{
    using ul_t = std::underlying_type<CashFilter>::type;
    return static_cast<CashFilter>(
        safe_cast<ul_t>(lhs) & safe_cast<ul_t>(rhs));
}


class CashDiff
{
public:
    CashDiff() = delete;
    CashDiff (CashDiff const&) = delete;
    CashDiff (CashDiff&& other) noexcept;
    CashDiff& operator= (CashDiff const&) = delete;
    ~CashDiff();

    CashDiff (ReadView const& view,
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs);

    std::size_t commonCount () const;

    std::size_t rhsOnlyCount () const;

    std::size_t lhsOnlyCount () const;

    bool hasDiff() const;

    int xrpRoundToZero() const;

    bool rmDust();

    bool rmLhsDeletedOffers();
    bool rmRhsDeletedOffers();

    struct OfferAmounts
    {
        static std::size_t constexpr count_ = 2;
        static std::size_t constexpr count() { return count_; }
        STAmount amounts[count_];
        STAmount const& takerPays() const { return amounts[0]; }
        STAmount const& takerGets() const { return amounts[1]; }
        STAmount const& operator[] (std::size_t i) const
        {
            assert (i < count());
            return amounts[i];
        }
        friend bool operator< (OfferAmounts const& lhs, OfferAmounts const& rhs)
        {
            if (lhs[0] < rhs[0])
                return true;
            if (lhs[0] > rhs[0])
                return false;
            return lhs[1] < rhs[1];
        }
    };

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

bool diffIsDust (STAmount const& v1, STAmount const& v2, std::uint8_t e10 = 6);

} 

#endif








