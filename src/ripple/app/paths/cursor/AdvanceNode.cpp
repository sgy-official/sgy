

#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/View.h>

namespace ripple {
namespace path {

TER PathCursor::advanceNode (STAmount const& amount, bool reverse, bool callerHasLiquidity) const
{
    bool const multi = fix1141 (view ().info ().parentCloseTime)
        ? (multiQuality_ || (!callerHasLiquidity && amount == beast::zero))
        : (multiQuality_ || amount == beast::zero);

    if (multi == multiQuality_)
        return advanceNode (reverse);

    PathCursor withMultiQuality {rippleCalc_, pathState_, multi, j_, nodeIndex_};
    return withMultiQuality.advanceNode (reverse);
}

TER PathCursor::advanceNode (bool const bReverse) const
{
    TER resultCode = tesSUCCESS;

    JLOG (j_.trace())
            << "advanceNode: TakerPays:"
            << node().saTakerPays << " TakerGets:" << node().saTakerGets;

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    do
    {
        if (++loopCount > NODE_ADVANCE_MAX_LOOPS)
        {
            JLOG (j_.warn()) << "Loop count exceeded";
            return tefEXCEPTION;
        }

        bool bDirectDirDirty = node().directory.initialize (
            { previousNode().issue_, node().issue_},
            view());

        if (auto advance = node().directory.advance (view()))
        {
            bDirectDirDirty = true;
            if (advance == NodeDirectory::NEW_QUALITY)
            {
                JLOG (j_.trace())
                    << "advanceNode: Quality advance: node.directory.current="
                    << node().directory.current;
            }
            else if (bReverse)
            {
                JLOG (j_.trace())
                    << "advanceNode: No more offers.";

                node().offerIndex_ = beast::zero;
                break;
            }
            else
            {
                JLOG (j_.warn())
                    << "advanceNode: Unreachable: "
                    << "Fell off end of order book.";
                return telFAILED_PROCESSING;
            }
        }

        if (bDirectDirDirty)
        {
            node().saOfrRate = amountFromQuality (
                getQuality (node().directory.current));
            node().uEntry = 0;
            node().bEntryAdvance   = true;

            JLOG (j_.trace())
                << "advanceNode: directory dirty: node.saOfrRate="
                << node().saOfrRate;
        }

        if (!node().bEntryAdvance)
        {
            if (node().bFundsDirty)
            {
                node().saTakerPays
                        = node().sleOffer->getFieldAmount (sfTakerPays);
                node().saTakerGets
                        = node().sleOffer->getFieldAmount (sfTakerGets);

                node().saOfferFunds = accountFunds(view(),
                    node().offerOwnerAccount_,
                    node().saTakerGets,
                    fhZERO_IF_FROZEN, viewJ);
                node().bFundsDirty = false;

                JLOG (j_.trace())
                    << "advanceNode: funds dirty: node().saOfrRate="
                    << node().saOfrRate;
            }
            else
            {
                JLOG (j_.trace()) << "advanceNode: as is";
            }
        }
        else if (!dirNext (view(),
            node().directory.current,
            node().directory.ledgerEntry,
            node().uEntry,
            node().offerIndex_, viewJ))
        {
            if (multiQuality_)
            {
                JLOG (j_.trace())
                    << "advanceNode: next quality";
                node().directory.advanceNeeded  = true;  
            }
            else if (!bReverse)
            {
                JLOG (j_.warn())
                    << "advanceNode: unreachable: ran out of offers";
                return telFAILED_PROCESSING;
            }
            else
            {
                node().bEntryAdvance   = false;        
                node().offerIndex_ = beast::zero;      
            }
        }
        else
        {
            node().sleOffer = view().peek (keylet::offer(node().offerIndex_));

            if (!node().sleOffer)
            {
                JLOG (j_.warn()) <<
                    "Missing offer in directory";
                node().bEntryAdvance = true;
            }
            else
            {
                node().offerOwnerAccount_
                        = node().sleOffer->getAccountID (sfAccount);
                node().saTakerPays
                        = node().sleOffer->getFieldAmount (sfTakerPays);
                node().saTakerGets
                        = node().sleOffer->getFieldAmount (sfTakerGets);

                AccountIssue const accountIssue (
                    node().offerOwnerAccount_, node().issue_);

                JLOG (j_.trace())
                    << "advanceNode: offerOwnerAccount_="
                    << to_string (node().offerOwnerAccount_)
                    << " node.saTakerPays=" << node().saTakerPays
                    << " node.saTakerGets=" << node().saTakerGets
                    << " node.offerIndex_=" << node().offerIndex_;

                if (node().sleOffer->isFieldPresent (sfExpiration) &&
                        (node().sleOffer->getFieldU32 (sfExpiration) <=
                            view().parentCloseTime().time_since_epoch().count()))
                {
                    JLOG (j_.trace())
                        << "advanceNode: expired offer";
                    rippleCalc_.permanentlyUnfundedOffers_.insert(
                        node().offerIndex_);
                    continue;
                }

                if (node().saTakerPays <= beast::zero || node().saTakerGets <= beast::zero)
                {
                    auto const index = node().offerIndex_;
                    if (bReverse)
                    {
                        JLOG (j_.warn())
                            << "advanceNode: PAST INTERNAL ERROR"
                            << " REVERSE: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

                        rippleCalc_.permanentlyUnfundedOffers_.insert (
                            node().offerIndex_);
                    }
                    else if (rippleCalc_.permanentlyUnfundedOffers_.find (index)
                             != rippleCalc_.permanentlyUnfundedOffers_.end ())
                    {
                        JLOG (j_.debug())
                            << "advanceNode: PAST INTERNAL ERROR "
                            << " FORWARD CONFIRM: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

                    }
                    else
                    {
                        JLOG (j_.warn())
                            << "advanceNode: INTERNAL ERROR"

                            <<" FORWARD NEWLY FOUND: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

                        resultCode = tefEXCEPTION;
                    }

                    continue;
                }

                auto itForward = pathState_.forward().find (accountIssue);
                const bool bFoundForward =
                        itForward != pathState_.forward().end ();

                if (bFoundForward &&
                    itForward->second != nodeIndex_ &&
                    node().offerOwnerAccount_ != node().issue_.account)
                {
                    JLOG (j_.trace())
                        << "advanceNode: temporarily unfunded offer"
                        << " (forward)";
                    continue;
                }

                auto itReverse = pathState_.reverse().find (accountIssue);
                bool bFoundReverse = itReverse != pathState_.reverse().end ();

                if (bFoundReverse &&
                    itReverse->second != nodeIndex_ &&
                    node().offerOwnerAccount_ != node().issue_.account)
                {
                    JLOG (j_.trace())
                        << "advanceNode: temporarily unfunded offer"
                        <<" (reverse)";
                    continue;
                }

                auto itPast = rippleCalc_.mumSource_.find (accountIssue);
                bool bFoundPast = (itPast != rippleCalc_.mumSource_.end ());


                node().saOfferFunds = accountFunds(view(),
                    node().offerOwnerAccount_,
                    node().saTakerGets,
                    fhZERO_IF_FROZEN, viewJ);

                if (node().saOfferFunds <= beast::zero)
                {
                    JLOG (j_.trace())
                        << "advanceNode: unfunded offer";

                    if (bReverse && !bFoundReverse && !bFoundPast)
                    {
                        rippleCalc_.permanentlyUnfundedOffers_.insert (node().offerIndex_);
                    }
                    else
                    {
                    }

                    continue;
                }

                if (bReverse            
                    && !bFoundPast      
                    && !bFoundReverse)  
                {
                    JLOG (j_.trace())
                        << "advanceNode: remember="
                        <<  node().offerOwnerAccount_
                        << "/"
                        << node().issue_;

                    pathState_.insertReverse (accountIssue, nodeIndex_);
                }

                node().bFundsDirty = false;
                node().bEntryAdvance = false;
            }
        }
    }
    while (resultCode == tesSUCCESS &&
           (node().bEntryAdvance || node().directory.advanceNeeded));

    if (resultCode == tesSUCCESS)
    {
        JLOG (j_.trace())
            << "advanceNode: node.offerIndex_=" << node().offerIndex_;
    }
    else
    {
        JLOG (j_.debug())
            << "advanceNode: resultCode=" << transToken (resultCode);
    }

    return resultCode;
}

}  
}  
























