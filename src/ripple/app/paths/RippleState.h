

#ifndef RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLESTATE_H_INCLUDED

#include <ripple/ledger/View.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <cstdint>
#include <memory> 

namespace ripple {


class RippleState
{
public:
    using pointer = std::shared_ptr <RippleState>;

public:
    RippleState () = delete;

    virtual ~RippleState() = default;

    static RippleState::pointer makeItem(
        AccountID const& accountID,
        std::shared_ptr<SLE const> sle);

    RippleState (std::shared_ptr<SLE const>&& sle,
        AccountID const& viewAccount);

    
    uint256
    key() const
    {
        return sle_->key();
    }


    AccountID const& getAccountID () const
    {
        return  mViewLowest ? mLowID : mHighID;
    }

    AccountID const& getAccountIDPeer () const
    {
        return !mViewLowest ? mLowID : mHighID;
    }

    bool getAuth () const
    {
        return mFlags & (mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getAuthPeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getNoRipple () const
    {
        return mFlags & (mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    bool getNoRipplePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    
    bool getFreeze () const
    {
        return mFlags & (mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    
    bool getFreezePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    STAmount const& getBalance () const
    {
        return mBalance;
    }

    STAmount const& getLimit () const
    {
        return  mViewLowest ? mLowLimit : mHighLimit;
    }

    STAmount const& getLimitPeer () const
    {
        return !mViewLowest ? mLowLimit : mHighLimit;
    }

    Rate const&
    getQualityIn () const
    {
        return mViewLowest ? lowQualityIn_ : highQualityIn_;
    }

    Rate const&
    getQualityOut () const
    {
        return mViewLowest ? lowQualityOut_ : highQualityOut_;
    }

    Json::Value getJson (int);

private:
    std::shared_ptr<SLE const> sle_;

    bool                            mViewLowest;

    std::uint32_t                   mFlags;

    STAmount const&                 mLowLimit;
    STAmount const&                 mHighLimit;

    AccountID const&                  mLowID;
    AccountID const&                  mHighID;

    Rate lowQualityIn_;
    Rate lowQualityOut_;
    Rate highQualityIn_;
    Rate highQualityOut_;

    STAmount                        mBalance;
};

std::vector <RippleState::pointer>
getRippleStateItems (AccountID const& accountID,
    ReadView const& view);

} 

#endif








