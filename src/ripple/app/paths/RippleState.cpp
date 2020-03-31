

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/RippleState.h>
#include <ripple/protocol/STAmount.h>
#include <cstdint>
#include <memory>

namespace ripple {

RippleState::pointer
RippleState::makeItem (
    AccountID const& accountID,
        std::shared_ptr<SLE const> sle)
{
    if (! sle || sle->getType () != ltRIPPLE_STATE)
        return {};
    return std::make_shared<RippleState>(
        std::move(sle), accountID);
}

RippleState::RippleState (
    std::shared_ptr<SLE const>&& sle,
        AccountID const& viewAccount)
    : sle_ (std::move(sle))
    , mFlags (sle_->getFieldU32 (sfFlags))
    , mLowLimit (sle_->getFieldAmount (sfLowLimit))
    , mHighLimit (sle_->getFieldAmount (sfHighLimit))
    , mLowID (mLowLimit.getIssuer ())
    , mHighID (mHighLimit.getIssuer ())
    , lowQualityIn_ (sle_->getFieldU32 (sfLowQualityIn))
    , lowQualityOut_ (sle_->getFieldU32 (sfLowQualityOut))
    , highQualityIn_ (sle_->getFieldU32 (sfHighQualityIn))
    , highQualityOut_ (sle_->getFieldU32 (sfHighQualityOut))
    , mBalance (sle_->getFieldAmount (sfBalance))
{
    mViewLowest = (mLowID == viewAccount);

    if (!mViewLowest)
        mBalance.negate ();
}

Json::Value RippleState::getJson (int)
{
    Json::Value ret (Json::objectValue);
    ret["low_id"] = to_string (mLowID);
    ret["high_id"] = to_string (mHighID);
    return ret;
}

std::vector <RippleState::pointer>
getRippleStateItems (AccountID const& accountID,
    ReadView const& view)
{
    std::vector <RippleState::pointer> items;
    forEachItem(view, accountID,
        [&items,&accountID](
        std::shared_ptr<SLE const> const&sleCur)
        {
             auto ret = RippleState::makeItem (accountID, sleCur);
             if (ret)
                items.push_back (ret);
        });

    return items;
}

} 
























