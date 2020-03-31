

#include <ripple/ledger/CashDiff.h>
#include <ripple/ledger/detail/ApplyStateTable.h>
#include <ripple/protocol/st.h>
#include <boost/container/static_vector.hpp>
#include <cassert>
#include <cstdlib>                                  

namespace ripple {
namespace detail {

struct CashSummary
{
    explicit CashSummary() = default;

    std::vector<std::pair<
        AccountID, XRPAmount>> xrpChanges;

    std::vector<std::pair<
        std::tuple<AccountID, AccountID, Currency>, STAmount>> trustChanges;

    std::vector<std::pair<
        std::tuple<AccountID, AccountID, Currency>, bool>> trustDeletions;

    std::vector<std::pair<
        std::tuple<AccountID, std::uint32_t>,
        CashDiff::OfferAmounts>> offerChanges;

    std::vector<std::pair<
        std::tuple<AccountID, std::uint32_t>,
        CashDiff::OfferAmounts>> offerDeletions;

    bool hasDiff () const
    {
        return !xrpChanges.empty()
            || !trustChanges.empty()
            || !trustDeletions.empty()
            || !offerChanges.empty()
            || !offerDeletions.empty();
    }

    void reserve (size_t newCap)
    {
        xrpChanges.reserve (newCap);
        trustChanges.reserve (newCap);
        trustDeletions.reserve (newCap);
        offerChanges.reserve (newCap);
        offerDeletions.reserve (newCap);
    }

    void shrink_to_fit()
    {
        xrpChanges.shrink_to_fit();
        trustChanges.shrink_to_fit();
        trustDeletions.shrink_to_fit();
        offerChanges.shrink_to_fit();
        offerDeletions.shrink_to_fit();
    }

    void sort()
    {
        std::sort (xrpChanges.begin(), xrpChanges.end());
        std::sort (trustChanges.begin(), trustChanges.end());
        std::sort (trustDeletions.begin(), trustDeletions.end());
        std::sort (offerChanges.begin(), offerChanges.end());
        std::sort (offerDeletions.begin(), offerDeletions.end());
    }
};

static bool treatZeroOfferAsDeletion (CashSummary& result, bool isDelete,
    std::shared_ptr<SLE const> const& before,
    std::shared_ptr<SLE const> const& after)
{
    using beast::zero;

    if (!before)
        return false;

    auto const& prev = *before;

    if (isDelete)
    {
        if (prev.getType() == ltOFFER &&
            prev[sfTakerPays] == zero && prev[sfTakerGets] == zero)
        {
            return true;
        }
    }
    else
    {
        if (!after)
            return false;

        auto const& cur = *after;
        if (cur.getType() == ltOFFER &&
            cur[sfTakerPays] == zero && cur[sfTakerGets] == zero)
        {
            auto const oldTakerPays = prev[sfTakerPays];
            auto const oldTakerGets = prev[sfTakerGets];
            if (oldTakerPays != zero && oldTakerGets != zero)
            {
                result.offerDeletions.push_back (
                    std::make_pair (
                        std::make_tuple (prev[sfAccount], prev[sfSequence]),
                    CashDiff::OfferAmounts {{oldTakerPays, oldTakerGets}}));
                return true;
            }
        }
    }
    return false;
}

static bool getBasicCashFlow (CashSummary& result, bool isDelete,
    std::shared_ptr<SLE const> const& before,
    std::shared_ptr<SLE const> const& after)
{
    if (isDelete)
    {
        if (!before)
            return false;

        auto const& prev = *before;
        switch(prev.getType())
        {
        case ltACCOUNT_ROOT:
            result.xrpChanges.push_back (
                std::make_pair (prev[sfAccount], XRPAmount {0}));
            return true;

        case ltRIPPLE_STATE:
            result.trustDeletions.push_back(
                std::make_pair (
                    std::make_tuple (
                        prev[sfLowLimit].getIssuer(),
                        prev[sfHighLimit].getIssuer(),
                        prev[sfBalance].getCurrency()), false));
            return true;

        case ltOFFER:
            result.offerDeletions.push_back (
                std::make_pair (
                    std::make_tuple (prev[sfAccount], prev[sfSequence]),
                CashDiff::OfferAmounts {{
                    prev[sfTakerPays],
                    prev[sfTakerGets]}}));
            return true;

        default:
            return false;
        }
    }
    else
    {
        if (!after)
        {
            assert (after);
            return false;
        }

        auto const& cur = *after;
        switch(cur.getType())
        {
        case ltACCOUNT_ROOT:
        {
            auto const curXrp = cur[sfBalance].xrp();
            if (!before || (*before)[sfBalance].xrp() != curXrp)
                result.xrpChanges.push_back (
                    std::make_pair (cur[sfAccount], curXrp));
            return true;
        }
        case ltRIPPLE_STATE:
        {
            auto const curBalance = cur[sfBalance];
            if (!before || (*before)[sfBalance] != curBalance)
                result.trustChanges.push_back (
                    std::make_pair (
                        std::make_tuple (
                            cur[sfLowLimit].getIssuer(),
                            cur[sfHighLimit].getIssuer(),
                            curBalance.getCurrency()),
                        curBalance));
            return true;
        }
        case ltOFFER:
        {
            auto const curTakerPays = cur[sfTakerPays];
            auto const curTakerGets = cur[sfTakerGets];
            if (!before || (*before)[sfTakerGets] != curTakerGets ||
                (*before)[sfTakerPays] != curTakerPays)
            {
                result.offerChanges.push_back (
                    std::make_pair (
                        std::make_tuple (cur[sfAccount], cur[sfSequence]),
                    CashDiff::OfferAmounts {{curTakerPays, curTakerGets}}));
            }
            return true;
        }
        default:
            break;
        }
    }
    return false;
}

static CashSummary
getCashFlow (ReadView const& view, CashFilter f, ApplyStateTable const& table)
{
    CashSummary result;
    result.reserve (table.size());

    using FuncType = decltype (&getBasicCashFlow);
    boost::container::static_vector<FuncType, 2> filters;

    if ((f & CashFilter::treatZeroOfferAsDeletion) != CashFilter::none)
        filters.push_back (treatZeroOfferAsDeletion);

    filters.push_back (&getBasicCashFlow);

    auto each = [&result, &filters](uint256 const& key, bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after) {
        auto discarded =
            std::find_if (filters.begin(), filters.end(),
                [&result, isDelete, &before, &after] (FuncType func) {
                    return func (result, isDelete, before, after);
        });
        (void) discarded;
    };

    table.visit (view, each);
    result.sort();
    result.shrink_to_fit();
    return result;
}

} 


class CashDiff::Impl
{
private:
    struct DropsGone
    {
        XRPAmount lhs;
        XRPAmount rhs;
    };

    ReadView const& view_;

    std::size_t commonKeys_ = 0;  
    std::size_t lhsKeys_ = 0;     
    std::size_t rhsKeys_ = 0;     
    boost::optional<DropsGone> dropsGone_;
    detail::CashSummary lhsDiffs_;
    detail::CashSummary rhsDiffs_;

public:
    Impl (ReadView const& view,
          CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
          CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
    : view_ (view)
    {
        findDiffs (lhsFilter, lhs, rhsFilter, rhs);
    }

    std::size_t commonCount() const
    {
        return commonKeys_;
    }

    std::size_t lhsOnlyCount() const
    {
        return lhsKeys_;
    }

    std::size_t rhsOnlyCount () const
    {
        return rhsKeys_;
    }

    bool hasDiff () const
    {
        return dropsGone_ != boost::none
            || lhsDiffs_.hasDiff()
            || rhsDiffs_.hasDiff();
    }

    int xrpRoundToZero () const;

    bool rmDust ();

    bool rmLhsDeletedOffers();
    bool rmRhsDeletedOffers();

private:
    void findDiffs (
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs);
};

template <typename T, typename U>
static std::array<std::size_t, 3> countKeys (
    std::vector<std::pair<T, U>> const& lhs,
    std::vector<std::pair<T, U>> const& rhs)
{
    std::array<std::size_t, 3> ret {};  

    auto lhsItr = lhs.cbegin();
    auto rhsItr = rhs.cbegin();

    while (lhsItr != lhs.cend() || rhsItr != rhs.cend())
    {
        if (lhsItr == lhs.cend())
        {
            ret[2] += 1;
            ++rhsItr;
        }
        else if (rhsItr == rhs.cend())
        {
            ret[1] += 1;
            ++lhsItr;
        }
        else if (lhsItr->first < rhsItr->first)
        {
            ret[1] += 1;
            ++lhsItr;
        }
        else if (rhsItr->first < lhsItr->first)
        {
            ret[2] += 1;
            ++rhsItr;
        }
        else
        {
            ret[0] += 1;
            ++lhsItr;
            ++rhsItr;
        }
    }
    return ret;
}

static std::array<std::size_t, 3>
countKeys (detail::CashSummary const& lhs, detail::CashSummary const& rhs)
{
    std::array<std::size_t, 3> ret {};  

    auto addIn = [&ret] (std::array<std::size_t, 3> const& a)
    {
        std::transform (a.cbegin(), a.cend(),
            ret.cbegin(), ret.begin(), std::plus<std::size_t>());
    };
    addIn (countKeys(lhs.xrpChanges,     rhs.xrpChanges));
    addIn (countKeys(lhs.trustChanges,   rhs.trustChanges));
    addIn (countKeys(lhs.trustDeletions, rhs.trustDeletions));
    addIn (countKeys(lhs.offerChanges,   rhs.offerChanges));
    addIn (countKeys(lhs.offerDeletions, rhs.offerDeletions));
    return ret;
}

int CashDiff::Impl::xrpRoundToZero () const
{
    if (lhsDiffs_.offerChanges.size() != 1 ||
        rhsDiffs_.offerChanges.size() != 1)
            return 0;

    if (! lhsDiffs_.offerChanges[0].second.takerGets().native() ||
        ! rhsDiffs_.offerChanges[0].second.takerGets().native())
            return 0;

    bool const lhsBigger =
        lhsDiffs_.offerChanges[0].second.takerGets().mantissa() >
        rhsDiffs_.offerChanges[0].second.takerGets().mantissa();

    detail::CashSummary const& bigger = lhsBigger ? lhsDiffs_ : rhsDiffs_;
    detail::CashSummary const& smaller = lhsBigger ? rhsDiffs_ : lhsDiffs_;
    if (bigger.offerChanges[0].second.takerGets().mantissa() -
        smaller.offerChanges[0].second.takerGets().mantissa() != 1)
           return 0;

    if (smaller.xrpChanges.size() != 2)
        return 0;
    if (! bigger.xrpChanges.empty())
        return 0;

    if (!smaller.trustChanges.empty()   || !bigger.trustChanges.empty()   ||
        !smaller.trustDeletions.empty() || !bigger.trustDeletions.empty() ||
        !smaller.offerDeletions.empty() || !bigger.offerDeletions.empty())
            return 0;

    return lhsBigger ? -1 : 1;
}

static bool diffIsDust (
    CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
{
    for (auto i = 0; i < lhs.count(); ++i)
    {
        if (!diffIsDust (lhs[i], rhs[i]))
            return false;
    }
    return true;
}

template <typename T, typename U, typename L>
static bool
rmVecDust (
    std::vector<std::pair<T, U>>& lhs,
    std::vector<std::pair<T, U>>& rhs,
    L&& justDust)
{
    static_assert (
        std::is_same<bool,
        decltype (justDust (lhs[0].second, rhs[0].second))>::value,
        "Invalid lambda passed to rmVecDust");

    bool dustWasRemoved = false;
    auto lhsItr = lhs.begin();
    while (lhsItr != lhs.end())
    {
        using value_t = std::pair<T, U>;
        auto const found = std::equal_range (rhs.begin(), rhs.end(), *lhsItr,
            [] (value_t const& a, value_t const& b)
            {
                return a.first < b.first;
            });

        if (found.first != found.second)
        {
            if (justDust (lhsItr->second, found.first->second))
            {
                dustWasRemoved = true;
                rhs.erase (found.first);
                lhsItr = lhs.erase (lhsItr);
                continue;
            }
        }
        ++lhsItr;
    }
    return dustWasRemoved;
}

bool CashDiff::Impl::rmDust ()
{
    bool removedDust = false;


    removedDust |= rmVecDust (lhsDiffs_.xrpChanges, rhsDiffs_.xrpChanges,
        [](XRPAmount const& lhs, XRPAmount const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

    removedDust |= rmVecDust (lhsDiffs_.trustChanges, rhsDiffs_.trustChanges,
        [](STAmount const& lhs, STAmount const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

    removedDust |= rmVecDust (lhsDiffs_.offerChanges, rhsDiffs_.offerChanges,
        [](CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

    removedDust |= rmVecDust (
        lhsDiffs_.offerDeletions, rhsDiffs_.offerDeletions,
        [](CashDiff::OfferAmounts const& lhs, CashDiff::OfferAmounts const& rhs)
        {
            return diffIsDust (lhs, rhs);
        });

    return removedDust;
}

bool CashDiff::Impl::rmLhsDeletedOffers()
{
    bool const ret = !lhsDiffs_.offerDeletions.empty();
    if (ret)
        lhsDiffs_.offerDeletions.clear();
    return ret;
}

bool CashDiff::Impl::rmRhsDeletedOffers()
{
    bool const ret = !rhsDiffs_.offerDeletions.empty();
    if (ret)
        rhsDiffs_.offerDeletions.clear();
    return ret;
}

template <typename T>
static void setDiff (
    std::vector<T> const& a, std::vector<T> const& b, std::vector<T>& dest)
{
    dest.clear();
    std::set_difference (a.cbegin(), a.cend(),
        b.cbegin(), b.cend(), std::inserter (dest, dest.end()));
}

void CashDiff::Impl::findDiffs (
        CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
        CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
{
    if (lhs.dropsDestroyed() != rhs.dropsDestroyed())
    {
        dropsGone_ = DropsGone{lhs.dropsDestroyed(), rhs.dropsDestroyed()};
    }

    auto lhsDiffs = getCashFlow (view_, lhsFilter, lhs);
    auto rhsDiffs = getCashFlow (view_, rhsFilter, rhs);

    auto const counts = countKeys (lhsDiffs, rhsDiffs);
    commonKeys_ = counts[0];
    lhsKeys_    = counts[1];
    rhsKeys_    = counts[2];

    setDiff (lhsDiffs.xrpChanges, rhsDiffs.xrpChanges, lhsDiffs_.xrpChanges);
    setDiff (rhsDiffs.xrpChanges, lhsDiffs.xrpChanges, rhsDiffs_.xrpChanges);

    setDiff (lhsDiffs.trustChanges, rhsDiffs.trustChanges, lhsDiffs_.trustChanges);
    setDiff (rhsDiffs.trustChanges, lhsDiffs.trustChanges, rhsDiffs_.trustChanges);

    setDiff (lhsDiffs.trustDeletions, rhsDiffs.trustDeletions, lhsDiffs_.trustDeletions);
    setDiff (rhsDiffs.trustDeletions, lhsDiffs.trustDeletions, rhsDiffs_.trustDeletions);

    setDiff (lhsDiffs.offerChanges, rhsDiffs.offerChanges, lhsDiffs_.offerChanges);
    setDiff (rhsDiffs.offerChanges, lhsDiffs.offerChanges, rhsDiffs_.offerChanges);

    setDiff (lhsDiffs.offerDeletions, rhsDiffs.offerDeletions, lhsDiffs_.offerDeletions);
    setDiff (rhsDiffs.offerDeletions, lhsDiffs.offerDeletions, rhsDiffs_.offerDeletions);
}


CashDiff::CashDiff (CashDiff&& other) noexcept
: impl_ (std::move (other.impl_))
{
}

CashDiff::~CashDiff() = default;

CashDiff::CashDiff (ReadView const& view,
    CashFilter lhsFilter, detail::ApplyStateTable const& lhs,
    CashFilter rhsFilter, detail::ApplyStateTable const& rhs)
: impl_ (new Impl (view, lhsFilter, lhs, rhsFilter, rhs))
{
}

std::size_t CashDiff::commonCount () const
{
    return impl_->commonCount();
}

std::size_t CashDiff::rhsOnlyCount () const
{
    return impl_->rhsOnlyCount();
}

std::size_t CashDiff::lhsOnlyCount () const
{
    return impl_->lhsOnlyCount();
}

bool CashDiff::hasDiff() const
{
    return impl_->hasDiff();
}

int CashDiff::xrpRoundToZero() const
{
    return impl_->xrpRoundToZero();
}

bool CashDiff::rmDust()
{
    return impl_->rmDust();
}

bool CashDiff::rmLhsDeletedOffers()
{
    return impl_->rmLhsDeletedOffers();
}

bool CashDiff::rmRhsDeletedOffers()
{
    return impl_->rmRhsDeletedOffers();
}


bool diffIsDust (STAmount const& v1, STAmount const& v2, std::uint8_t e10)
{
    if (v1 != beast::zero && v2 != beast::zero && (v1.negative() != v2.negative()))
        return false;

    if (v1.native() != v2.native())
        return false;

    if (!v1.native() && (v1.issue() != v2.issue()))
        return false;

    if (v1 == v2)
        return true;

    STAmount const& small = v1 < v2 ? v1 : v2;
    STAmount const& large = v1 < v2 ? v2 : v1;

    if (v1.native())
    {
        std::uint64_t const s = small.mantissa();
        std::uint64_t const l = large.mantissa();

        if (l - s <= 2)
            return true;

        static_assert (sizeof (1ULL) == sizeof (std::uint64_t), "");
        std::uint64_t const ratio = s / (l - s);
        static constexpr std::uint64_t e10Lookup[]
        {
                                     1ULL,
                                    10ULL,
                                   100ULL,
                                 1'000ULL,
                                10'000ULL,
                               100'000ULL,
                             1'000'000ULL,
                            10'000'000ULL,
                           100'000'000ULL,
                         1'000'000'000ULL,
                        10'000'000'000ULL,
                       100'000'000'000ULL,
                     1'000'000'000'000ULL,
                    10'000'000'000'000ULL,
                   100'000'000'000'000ULL,
                 1'000'000'000'000'000ULL,
                10'000'000'000'000'000ULL,
               100'000'000'000'000'000ULL,
             1'000'000'000'000'000'000ULL,
            10'000'000'000'000'000'000ULL,
        };
        static std::size_t constexpr maxIndex =
            sizeof (e10Lookup) / sizeof e10Lookup[0];

        static_assert (
            std::numeric_limits<std::uint64_t>::max() /
            e10Lookup[maxIndex - 1] < 10, "Table too small");

        if (e10 >= maxIndex)
            return false;

        return ratio >= e10Lookup[e10];
    }

    STAmount const diff = large - small;
    if (diff == beast::zero)
        return true;

    STAmount const ratio = divide (small, diff, v1.issue());
    STAmount const one (v1.issue(), 1);
    int const ratioExp = ratio.exponent() - one.exponent();

    return ratioExp >= e10;
};

} 
























