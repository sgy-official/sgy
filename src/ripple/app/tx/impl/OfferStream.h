

#ifndef RIPPLE_APP_BOOK_OFFERSTREAM_H_INCLUDED
#define RIPPLE_APP_BOOK_OFFERSTREAM_H_INCLUDED

#include <ripple/app/tx/impl/BookTip.h>
#include <ripple/app/tx/impl/Offer.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Quality.h>
#include <ripple/beast/utility/Journal.h>

#include <boost/container/flat_set.hpp>

namespace ripple {

template<class TIn, class TOut>
class TOfferStreamBase
{
public:
    class StepCounter
    {
    private:
        std::uint32_t const limit_;
        std::uint32_t count_;
        beast::Journal j_;

    public:
        StepCounter (std::uint32_t limit, beast::Journal j)
            : limit_ (limit)
            , count_ (0)
            , j_ (j)
        {
        }

        bool
        step ()
        {
            if (count_ >= limit_)
            {
                JLOG(j_.debug()) << "Exceeded " << limit_ << " step limit.";
                return false;
            }
            count_++;
            return true;
        }
        std::uint32_t count() const
        {
            return count_;
        }
    };

protected:
    beast::Journal j_;
    ApplyView& view_;
    ApplyView& cancelView_;
    Book book_;
    NetClock::time_point const expire_;
    BookTip tip_;
    TOffer<TIn, TOut> offer_;
    boost::optional<TOut> ownerFunds_;
    StepCounter& counter_;

    void
    erase (ApplyView& view);

    virtual
    void
    permRmOffer (uint256 const& offerIndex) = 0;

public:
    TOfferStreamBase (ApplyView& view, ApplyView& cancelView,
        Book const& book, NetClock::time_point when,
            StepCounter& counter, beast::Journal journal);

    virtual ~TOfferStreamBase() = default;

    
    TOffer<TIn, TOut>&
    tip () const
    {
        return const_cast<TOfferStreamBase*>(this)->offer_;
    }

    
    bool
    step ();

    TOut ownerFunds () const
    {
        return *ownerFunds_;
    }
};


class OfferStream : public TOfferStreamBase<STAmount, STAmount>
{
protected:
    void
    permRmOffer (uint256 const& offerIndex) override;
public:
    using TOfferStreamBase<STAmount, STAmount>::TOfferStreamBase;
};


template <class TIn, class TOut>
class FlowOfferStream : public TOfferStreamBase<TIn, TOut>
{
private:
    boost::container::flat_set<uint256> permToRemove_;

public:
    using TOfferStreamBase<TIn, TOut>::TOfferStreamBase;

    void
    permRmOffer (uint256 const& offerIndex) override;

    boost::container::flat_set<uint256> const& permToRemove () const
    {
        return permToRemove_;
    }
};
}

#endif









