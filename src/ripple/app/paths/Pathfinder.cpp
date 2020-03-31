

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/Tuning.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/Config.h>
#include <tuple>



namespace ripple {

namespace {

struct AccountCandidate
{
    int priority;
    AccountID account;

    static const int highPriority = 10000;
};

bool compareAccountCandidate (
    std::uint32_t seq,
    AccountCandidate const& first, AccountCandidate const& second)
{
    if (first.priority < second.priority)
        return false;

    if (first.account > second.account)
        return true;

    return (first.priority ^ seq) < (second.priority ^ seq);
}

using AccountCandidates = std::vector<AccountCandidate>;

struct CostedPath
{
    int searchLevel;
    Pathfinder::PathType type;
};

using CostedPathList = std::vector<CostedPath>;

using PathTable = std::map<Pathfinder::PaymentType, CostedPathList>;

struct PathCost {
    int cost;
    char const* path;
};
using PathCostList = std::vector<PathCost>;

static PathTable mPathTable;

std::string pathTypeToString (Pathfinder::PathType const& type)
{
    std::string ret;

    for (auto const& node : type)
    {
        switch (node)
        {
            case Pathfinder::nt_SOURCE:
                ret.append("s");
                break;
            case Pathfinder::nt_ACCOUNTS:
                ret.append("a");
                break;
            case Pathfinder::nt_BOOKS:
                ret.append("b");
                break;
            case Pathfinder::nt_XRP_BOOK:
                ret.append("x");
                break;
            case Pathfinder::nt_DEST_BOOK:
                ret.append("f");
                break;
            case Pathfinder::nt_DESTINATION:
                ret.append("d");
                break;
        }
    }

    return ret;
}

}  

Pathfinder::Pathfinder (
    std::shared_ptr<RippleLineCache> const& cache,
    AccountID const& uSrcAccount,
    AccountID const& uDstAccount,
    Currency const& uSrcCurrency,
    boost::optional<AccountID> const& uSrcIssuer,
    STAmount const& saDstAmount,
    boost::optional<STAmount> const& srcAmount,
    Application& app)
    :   mSrcAccount (uSrcAccount),
        mDstAccount (uDstAccount),
        mEffectiveDst (isXRP(saDstAmount.getIssuer ()) ?
            uDstAccount : saDstAmount.getIssuer ()),
        mDstAmount (saDstAmount),
        mSrcCurrency (uSrcCurrency),
        mSrcIssuer (uSrcIssuer),
        mSrcAmount (srcAmount.value_or(STAmount({uSrcCurrency,
            uSrcIssuer.value_or(isXRP(uSrcCurrency) ?
                xrpAccount() : uSrcAccount)}, 1u, 0, true))),
        convert_all_ (mDstAmount ==
            STAmount(mDstAmount.issue(), STAmount::cMaxValue, STAmount::cMaxOffset)),
        mLedger (cache->getLedger ()),
        mRLCache (cache),
        app_ (app),
        j_ (app.journal ("Pathfinder"))
{
    assert (! uSrcIssuer || isXRP(uSrcCurrency) == isXRP(uSrcIssuer.get()));
}

bool Pathfinder::findPaths (int searchLevel)
{
    if (mDstAmount == beast::zero)
    {
        JLOG (j_.debug()) << "Destination amount was zero.";
        mLedger.reset ();
        return false;

    }

    if (mSrcAccount == mDstAccount &&
        mDstAccount == mEffectiveDst &&
        mSrcCurrency == mDstAmount.getCurrency ())
    {
        JLOG (j_.debug()) << "Tried to send to same issuer";
        mLedger.reset ();
        return false;
    }

    if (mSrcAccount == mEffectiveDst &&
        mSrcCurrency == mDstAmount.getCurrency())
    {
        return true;
    }

    m_loadEvent = app_.getJobQueue ().makeLoadEvent (
        jtPATH_FIND, "FindPath");
    auto currencyIsXRP = isXRP (mSrcCurrency);

    bool useIssuerAccount
            = mSrcIssuer && !currencyIsXRP && !isXRP (*mSrcIssuer);
    auto& account = useIssuerAccount ? *mSrcIssuer : mSrcAccount;
    auto issuer = currencyIsXRP ? AccountID() : account;
    mSource = STPathElement (account, mSrcCurrency, issuer);
    auto issuerString = mSrcIssuer
            ? to_string (*mSrcIssuer) : std::string ("none");
    JLOG (j_.trace())
            << "findPaths>"
            << " mSrcAccount=" << mSrcAccount
            << " mDstAccount=" << mDstAccount
            << " mDstAmount=" << mDstAmount.getFullText ()
            << " mSrcCurrency=" << mSrcCurrency
            << " mSrcIssuer=" << issuerString;

    if (!mLedger)
    {
        JLOG (j_.debug()) << "findPaths< no ledger";
        return false;
    }

    bool bSrcXrp = isXRP (mSrcCurrency);
    bool bDstXrp = isXRP (mDstAmount.getCurrency());

    if (! mLedger->exists (keylet::account(mSrcAccount)))
    {
        JLOG (j_.debug()) << "invalid source account";
        return false;
    }

    if ((mEffectiveDst != mDstAccount) &&
        ! mLedger->exists (keylet::account(mEffectiveDst)))
    {
        JLOG (j_.debug())
            << "Non-existent gateway";
        return false;
    }

    if (! mLedger->exists (keylet::account (mDstAccount)))
    {
        if (!bDstXrp)
        {
            JLOG (j_.debug())
                    << "New account not being funded in XRP ";
            return false;
        }

        auto const reserve = STAmount (mLedger->fees().accountReserve (0));
        if (mDstAmount < reserve)
        {
            JLOG (j_.debug())
                    << "New account not getting enough funding: "
                    << mDstAmount << " < " << reserve;
            return false;
        }
    }

    PaymentType paymentType;
    if (bSrcXrp && bDstXrp)
    {
        JLOG (j_.debug()) << "XRP to XRP payment";
        paymentType = pt_XRP_to_XRP;
    }
    else if (bSrcXrp)
    {
        JLOG (j_.debug()) << "XRP to non-XRP payment";
        paymentType = pt_XRP_to_nonXRP;
    }
    else if (bDstXrp)
    {
        JLOG (j_.debug()) << "non-XRP to XRP payment";
        paymentType = pt_nonXRP_to_XRP;
    }
    else if (mSrcCurrency == mDstAmount.getCurrency ())
    {
        JLOG (j_.debug()) << "non-XRP to non-XRP - same currency";
        paymentType = pt_nonXRP_to_same;
    }
    else
    {
        JLOG (j_.debug()) << "non-XRP to non-XRP - cross currency";
        paymentType = pt_nonXRP_to_nonXRP;
    }

    for (auto const& costedPath : mPathTable[paymentType])
    {
        if (costedPath.searchLevel <= searchLevel)
        {
            addPathsForType (costedPath.type);

            if (mCompletePaths.size () > PATHFINDER_MAX_COMPLETE_PATHS)
                break;
        }
    }

    JLOG (j_.debug())
            << mCompletePaths.size () << " complete paths found";

    return true;
}

TER Pathfinder::getPathLiquidity (
    STPath const& path,            
    STAmount const& minDstAmount,  
    STAmount& amountOut,           
    uint64_t& qualityOut) const    
{
    STPathSet pathSet;
    pathSet.push_back (path);

    path::RippleCalc::Input rcInput;
    rcInput.defaultPathsAllowed = false;

    PaymentSandbox sandbox (&*mLedger, tapNONE);

    try
    {
        if (convert_all_)
            rcInput.partialPaymentAllowed = true;

        auto rc = path::RippleCalc::rippleCalculate (
            sandbox,
            mSrcAmount,
            minDstAmount,
            mDstAccount,
            mSrcAccount,
            pathSet,
            app_.logs(),
            &rcInput);
        if (rc.result () != tesSUCCESS)
            return rc.result ();

        qualityOut = getRate (rc.actualAmountOut, rc.actualAmountIn);
        amountOut = rc.actualAmountOut;

        if (! convert_all_)
        {
            rcInput.partialPaymentAllowed = true;
            rc = path::RippleCalc::rippleCalculate (
                sandbox,
                mSrcAmount,
                mDstAmount - amountOut,
                mDstAccount,
                mSrcAccount,
                pathSet,
                app_.logs (),
                &rcInput);

            if (rc.result () == tesSUCCESS)
                amountOut += rc.actualAmountOut;
        }

        return tesSUCCESS;
    }
    catch (std::exception const& e)
    {
        JLOG (j_.info()) <<
            "checkpath: exception (" << e.what() << ") " <<
            path.getJson (JsonOptions::none);
        return tefEXCEPTION;
    }
}

namespace {

STAmount smallestUsefulAmount (STAmount const& amount, int maxPaths)
{
    return divide (amount, STAmount (maxPaths + 2), amount.issue ());
}

} 

void Pathfinder::computePathRanks (int maxPaths)
{
    mRemainingAmount = convert_all_ ?
        STAmount(mDstAmount.issue(), STAmount::cMaxValue,
            STAmount::cMaxOffset)
        : mDstAmount;

    try
    {
        PaymentSandbox sandbox (&*mLedger, tapNONE);

        path::RippleCalc::Input rcInput;
        rcInput.partialPaymentAllowed = true;
        auto rc = path::RippleCalc::rippleCalculate (
            sandbox,
            mSrcAmount,
            mRemainingAmount,
            mDstAccount,
            mSrcAccount,
            STPathSet(),
            app_.logs (),
            &rcInput);

        if (rc.result () == tesSUCCESS)
        {
            JLOG (j_.debug())
                    << "Default path contributes: " << rc.actualAmountIn;
            mRemainingAmount -= rc.actualAmountOut;
        }
        else
        {
            JLOG (j_.debug())
                << "Default path fails: " << transToken (rc.result ());
        }
    }
    catch (std::exception const&)
    {
        JLOG (j_.debug()) << "Default path causes exception";
    }

    rankPaths (maxPaths, mCompletePaths, mPathRanks);
}

static bool isDefaultPath (STPath const& path)
{
    return path.size() == 1;
}

static STPath removeIssuer (STPath const& path)
{
    STPath ret;

    for (auto it = path.begin() + 1; it != path.end(); ++it)
        ret.push_back (*it);

    return ret;
}

void Pathfinder::rankPaths (
    int maxPaths,
    STPathSet const& paths,
    std::vector <PathRank>& rankedPaths)
{
    rankedPaths.clear ();
    rankedPaths.reserve (paths.size());

    STAmount saMinDstAmount;
    if (convert_all_)
    {
        saMinDstAmount = STAmount(mDstAmount.issue(),
            STAmount::cMaxValue, STAmount::cMaxOffset);
    }
    else
    {
        saMinDstAmount = smallestUsefulAmount(mDstAmount, maxPaths);
    }

    for (int i = 0; i < paths.size (); ++i)
    {
        auto const& currentPath = paths[i];
        if (! currentPath.empty())
        {
            STAmount liquidity;
            uint64_t uQuality;
            auto const resultCode = getPathLiquidity (
                currentPath, saMinDstAmount, liquidity, uQuality);
            if (resultCode != tesSUCCESS)
            {
                JLOG (j_.debug()) <<
                    "findPaths: dropping : " <<
                    transToken (resultCode) <<
                    ": " << currentPath.getJson (JsonOptions::none);
            }
            else
            {
                JLOG (j_.debug()) <<
                    "findPaths: quality: " << uQuality <<
                    ": " << currentPath.getJson (JsonOptions::none);

                rankedPaths.push_back ({uQuality,
                    currentPath.size (), liquidity, i});
            }
        }
    }

    std::sort(rankedPaths.begin(), rankedPaths.end(),
        [&](Pathfinder::PathRank const& a, Pathfinder::PathRank const& b)
    {
        if (! convert_all_ && a.quality != b.quality)
            return a.quality < b.quality;

        if (a.liquidity != b.liquidity)
            return a.liquidity > b.liquidity;

        if (a.length != b.length)
            return a.length < b.length;

        return a.index > b.index;
    });
}

STPathSet
Pathfinder::getBestPaths (
    int maxPaths,
    STPath& fullLiquidityPath,
    STPathSet const& extraPaths,
    AccountID const& srcIssuer)
{
    JLOG (j_.debug()) << "findPaths: " <<
        mCompletePaths.size() << " paths and " <<
        extraPaths.size () << " extras";

    if (mCompletePaths.empty() && extraPaths.empty())
        return mCompletePaths;

    assert (fullLiquidityPath.empty ());
    const bool issuerIsSender = isXRP (mSrcCurrency) || (srcIssuer == mSrcAccount);

    std::vector <PathRank> extraPathRanks;
    rankPaths (maxPaths, extraPaths, extraPathRanks);

    STPathSet bestPaths;

    STAmount remaining = mRemainingAmount;

    auto pathsIterator = mPathRanks.begin();
    auto extraPathsIterator = extraPathRanks.begin();

    while (pathsIterator != mPathRanks.end() ||
        extraPathsIterator != extraPathRanks.end())
    {
        bool usePath = false;
        bool useExtraPath = false;

        if (pathsIterator == mPathRanks.end())
            useExtraPath = true;
        else if (extraPathsIterator == extraPathRanks.end())
            usePath = true;
        else if (extraPathsIterator->quality < pathsIterator->quality)
            useExtraPath = true;
        else if (extraPathsIterator->quality > pathsIterator->quality)
            usePath = true;
        else if (extraPathsIterator->liquidity > pathsIterator->liquidity)
            useExtraPath = true;
        else if (extraPathsIterator->liquidity < pathsIterator->liquidity)
            usePath = true;
        else
        {
            useExtraPath = true;
            usePath = true;
        }

        auto& pathRank = usePath ? *pathsIterator : *extraPathsIterator;

        auto const& path = usePath ? mCompletePaths[pathRank.index] :
            extraPaths[pathRank.index];

        if (useExtraPath)
            ++extraPathsIterator;

        if (usePath)
            ++pathsIterator;

        auto iPathsLeft = maxPaths - bestPaths.size ();
        if (!(iPathsLeft > 0 || fullLiquidityPath.empty ()))
            break;

        if (path.empty ())
        {
            assert (false);
            continue;
        }

        bool startsWithIssuer = false;

        if (! issuerIsSender && usePath)
        {
            if (isDefaultPath(path) ||
                path.front().getAccountID() != srcIssuer)
            {
                continue;
            }

            startsWithIssuer = true;
        }

        if (iPathsLeft > 1 ||
            (iPathsLeft > 0 && pathRank.liquidity >= remaining))
        {
            --iPathsLeft;
            remaining -= pathRank.liquidity;
            bestPaths.push_back (startsWithIssuer ? removeIssuer (path) : path);
        }
        else if (iPathsLeft == 0 &&
                 pathRank.liquidity >= mDstAmount &&
                 fullLiquidityPath.empty ())
        {
            fullLiquidityPath = (startsWithIssuer ? removeIssuer (path) : path);
            JLOG (j_.debug()) <<
                "Found extra full path: " <<
                fullLiquidityPath.getJson (JsonOptions::none);
        }
        else
        {
            JLOG (j_.debug()) <<
                "Skipping a non-filling path: " <<
                path.getJson (JsonOptions::none);
        }
    }

    if (remaining > beast::zero)
    {
        assert (fullLiquidityPath.empty ());
        JLOG (j_.info()) <<
            "Paths could not send " << remaining << " of " << mDstAmount;
    }
    else
    {
        JLOG (j_.debug()) <<
            "findPaths: RESULTS: " <<
            bestPaths.getJson (JsonOptions::none);
    }
    return bestPaths;
}

bool Pathfinder::issueMatchesOrigin (Issue const& issue)
{
    bool matchingCurrency = (issue.currency == mSrcCurrency);
    bool matchingAccount =
            isXRP (issue.currency) ||
            (mSrcIssuer && issue.account == mSrcIssuer) ||
            issue.account == mSrcAccount;

    return matchingCurrency && matchingAccount;
}

int Pathfinder::getPathsOut (
    Currency const& currency,
    AccountID const& account,
    bool isDstCurrency,
    AccountID const& dstAccount)
{
    Issue const issue (currency, account);

    auto it = mPathsOutCountMap.emplace (issue, 0);

    if (!it.second)
        return it.first->second;

    auto sleAccount = mLedger->read(keylet::account (account));

    if (!sleAccount)
        return 0;

    int aFlags = sleAccount->getFieldU32 (sfFlags);
    bool const bAuthRequired = (aFlags & lsfRequireAuth) != 0;
    bool const bFrozen = ((aFlags & lsfGlobalFreeze) != 0);

    int count = 0;

    if (!bFrozen)
    {
        count = app_.getOrderBookDB ().getBookSize (issue);

        for (auto const& item : mRLCache->getRippleLines (account))
        {
            RippleState* rspEntry = (RippleState*) item.get ();

            if (currency != rspEntry->getLimit ().getCurrency ())
            {
            }
            else if (rspEntry->getBalance () <= beast::zero &&
                     (!rspEntry->getLimitPeer ()
                      || -rspEntry->getBalance () >= rspEntry->getLimitPeer ()
                      ||  (bAuthRequired && !rspEntry->getAuth ())))
            {
            }
            else if (isDstCurrency &&
                     dstAccount == rspEntry->getAccountIDPeer ())
            {
                count += 10000; 
            }
            else if (rspEntry->getNoRipplePeer ())
            {
            }
            else if (rspEntry->getFreezePeer ())
            {
            }
            else
            {
                ++count;
            }
        }
    }
    it.first->second = count;
    return count;
}

void Pathfinder::addLinks (
    STPathSet const& currentPaths,  
    STPathSet& incompletePaths,     
    int addFlags)
{
    JLOG (j_.debug())
        << "addLink< on " << currentPaths.size ()
        << " source(s), flags=" << addFlags;
    for (auto const& path: currentPaths)
        addLink (path, incompletePaths, addFlags);
}

STPathSet& Pathfinder::addPathsForType (PathType const& pathType)
{
    auto it = mPaths.find (pathType);
    if (it != mPaths.end ())
        return it->second;

    if (pathType.empty ())
        return mPaths[pathType];

    PathType parentPathType = pathType;
    parentPathType.pop_back ();

    STPathSet const& parentPaths = addPathsForType (parentPathType);
    STPathSet& pathsOut = mPaths[pathType];

    JLOG (j_.debug())
        << "getPaths< adding onto '"
        << pathTypeToString (parentPathType) << "' to get '"
        << pathTypeToString (pathType) << "'";

    int initialSize = mCompletePaths.size ();

    auto nodeType = pathType.back ();
    switch (nodeType)
    {
    case nt_SOURCE:
        assert (pathsOut.empty ());
        pathsOut.push_back (STPath ());
        break;

    case nt_ACCOUNTS:
        addLinks (parentPaths, pathsOut, afADD_ACCOUNTS);
        break;

    case nt_BOOKS:
        addLinks (parentPaths, pathsOut, afADD_BOOKS);
        break;

    case nt_XRP_BOOK:
        addLinks (parentPaths, pathsOut, afADD_BOOKS | afOB_XRP);
        break;

    case nt_DEST_BOOK:
        addLinks (parentPaths, pathsOut, afADD_BOOKS | afOB_LAST);
        break;

    case nt_DESTINATION:
        addLinks (parentPaths, pathsOut, afADD_ACCOUNTS | afAC_LAST);
        break;
    }

    if (mCompletePaths.size () != initialSize)
    {
        JLOG (j_.debug())
            << (mCompletePaths.size () - initialSize)
            << " complete paths added";
    }

    JLOG (j_.debug())
        << "getPaths> " << pathsOut.size () << " partial paths found";
    return pathsOut;
}

bool Pathfinder::isNoRipple (
    AccountID const& fromAccount,
    AccountID const& toAccount,
    Currency const& currency)
{
    auto sleRipple = mLedger->read(keylet::line(
        toAccount, fromAccount, currency));

    auto const flag ((toAccount > fromAccount)
                     ? lsfHighNoRipple : lsfLowNoRipple);

    return sleRipple && (sleRipple->getFieldU32 (sfFlags) & flag);
}

bool Pathfinder::isNoRippleOut (STPath const& currentPath)
{
    if (currentPath.empty ())
        return false;

    STPathElement const& endElement = currentPath.back ();
    if (!(endElement.getNodeType () & STPathElement::typeAccount))
        return false;

    auto const& fromAccount = (currentPath.size () == 1)
        ? mSrcAccount
        : (currentPath.end () - 2)->getAccountID ();
    auto const& toAccount = endElement.getAccountID ();
    return isNoRipple (fromAccount, toAccount, endElement.getCurrency ());
}

void addUniquePath (STPathSet& pathSet, STPath const& path)
{
    for (auto const& p : pathSet)
    {
        if (p == path)
            return;
    }
    pathSet.push_back (path);
}

void Pathfinder::addLink (
    const STPath& currentPath,      
    STPathSet& incompletePaths,     
    int addFlags)
{
    auto const& pathEnd = currentPath.empty() ? mSource : currentPath.back ();
    auto const& uEndCurrency = pathEnd.getCurrency ();
    auto const& uEndIssuer = pathEnd.getIssuerID ();
    auto const& uEndAccount = pathEnd.getAccountID ();
    bool const bOnXRP = uEndCurrency.isZero ();

    bool const hasEffectiveDestination = mEffectiveDst != mDstAccount;

    JLOG (j_.trace()) << "addLink< flags="
                                   << addFlags << " onXRP=" << bOnXRP;
    JLOG (j_.trace()) << currentPath.getJson (JsonOptions::none);

    if (addFlags & afADD_ACCOUNTS)
    {
        if (bOnXRP)
        {
            if (mDstAmount.native () && !currentPath.empty ())
            { 
                JLOG (j_.trace())
                    << "complete path found ax: "
                    << currentPath.getJson(JsonOptions::none);
                addUniquePath (mCompletePaths, currentPath);
            }
        }
        else
        {
            auto const sleEnd = mLedger->read(keylet::account(uEndAccount));

            if (sleEnd)
            {
                bool const bRequireAuth (
                    sleEnd->getFieldU32 (sfFlags) & lsfRequireAuth);
                bool const bIsEndCurrency (
                    uEndCurrency == mDstAmount.getCurrency ());
                bool const bIsNoRippleOut (
                    isNoRippleOut (currentPath));
                bool const bDestOnly (
                    addFlags & afAC_LAST);

                auto& rippleLines (mRLCache->getRippleLines (uEndAccount));

                AccountCandidates candidates;
                candidates.reserve (rippleLines.size ());

                for (auto const& item : rippleLines)
                {
                    auto* rs = dynamic_cast<RippleState const *> (item.get ());
                    if (!rs)
                    {
                        JLOG (j_.error())
                                << "Couldn't decipher RippleState";
                        continue;
                    }
                    auto const& acct = rs->getAccountIDPeer ();

                    if (hasEffectiveDestination && (acct == mDstAccount))
                    {
                        continue;
                    }

                    bool bToDestination = acct == mEffectiveDst;

                    if (bDestOnly && !bToDestination)
                    {
                        continue;
                    }

                    if ((uEndCurrency == rs->getLimit ().getCurrency ()) &&
                        !currentPath.hasSeen (acct, uEndCurrency, acct))
                    {
                        if (rs->getBalance () <= beast::zero
                            && (!rs->getLimitPeer ()
                                || -rs->getBalance () >= rs->getLimitPeer ()
                                || (bRequireAuth && !rs->getAuth ())))
                        {
                        }
                        else if (bIsNoRippleOut && rs->getNoRipple ())
                        {
                        }
                        else if (bToDestination)
                        {
                            if (uEndCurrency == mDstAmount.getCurrency ())
                            {
                                if (!currentPath.empty ())
                                {
                                    JLOG (j_.trace())
                                            << "complete path found ae: "
                                            << currentPath.getJson (JsonOptions::none);
                                    addUniquePath
                                            (mCompletePaths, currentPath);
                                }
                            }
                            else if (!bDestOnly)
                            {
                                candidates.push_back (
                                    {AccountCandidate::highPriority, acct});
                            }
                        }
                        else if (acct == mSrcAccount)
                        {
                        }
                        else
                        {
                            int out = getPathsOut (
                                uEndCurrency,
                                acct,
                                bIsEndCurrency,
                                mEffectiveDst);
                            if (out)
                                candidates.push_back ({out, acct});
                        }
                    }
                }

                if (!candidates.empty())
                {
                    std::sort (candidates.begin (), candidates.end (),
                        std::bind(compareAccountCandidate,
                                  mLedger->seq(),
                                  std::placeholders::_1,
                                  std::placeholders::_2));

                    int count = candidates.size ();
                    if ((count > 10) && (uEndAccount != mSrcAccount))
                        count = 10;
                    else if (count > 50)
                        count = 50;

                    auto it = candidates.begin();
                    while (count-- != 0)
                    {
                        STPathElement pathElement (
                            STPathElement::typeAccount,
                            it->account,
                            uEndCurrency,
                            it->account);
                        incompletePaths.assembleAdd (currentPath, pathElement);
                        ++it;
                    }
                }

            }
            else
            {
                JLOG (j_.warn())
                    << "Path ends on non-existent issuer";
            }
        }
    }
    if (addFlags & afADD_BOOKS)
    {
        if (addFlags & afOB_XRP)
        {
            if (!bOnXRP && app_.getOrderBookDB ().isBookToXRP (
                    {uEndCurrency, uEndIssuer}))
            {
                STPathElement pathElement(
                    STPathElement::typeCurrency,
                    xrpAccount (),
                    xrpCurrency (),
                    xrpAccount ());
                incompletePaths.assembleAdd (currentPath, pathElement);
            }
        }
        else
        {
            bool bDestOnly = (addFlags & afOB_LAST) != 0;
            auto books = app_.getOrderBookDB ().getBooksByTakerPays(
                {uEndCurrency, uEndIssuer});
            JLOG (j_.trace())
                << books.size () << " books found from this currency/issuer";

            for (auto const& book : books)
            {
                if (!currentPath.hasSeen (
                        xrpAccount(),
                        book->getCurrencyOut (),
                        book->getIssuerOut ()) &&
                    !issueMatchesOrigin (book->book ().out) &&
                    (!bDestOnly ||
                     (book->getCurrencyOut () == mDstAmount.getCurrency ())))
                {
                    STPath newPath (currentPath);

                    if (book->getCurrencyOut().isZero())
                    { 

                        newPath.emplace_back (
                            STPathElement::typeCurrency,
                            xrpAccount (),
                            xrpCurrency (),
                            xrpAccount ());

                        if (mDstAmount.getCurrency ().isZero ())
                        {
                            JLOG (j_.trace())
                                << "complete path found bx: "
                                << currentPath.getJson(JsonOptions::none);
                            addUniquePath (mCompletePaths, newPath);
                        }
                        else
                            incompletePaths.push_back (newPath);
                    }
                    else if (!currentPath.hasSeen(
                        book->getIssuerOut (),
                        book->getCurrencyOut (),
                        book->getIssuerOut ()))
                    {
                        if ((newPath.size() >= 2) &&
                            (newPath.back().isAccount ()) &&
                            (newPath[newPath.size() - 2].isOffer ()))
                        {
                            newPath[newPath.size() - 1] = STPathElement (
                                STPathElement::typeCurrency | STPathElement::typeIssuer,
                                xrpAccount(), book->getCurrencyOut(),
                                book->getIssuerOut());
                        }
                        else
                        {
                            newPath.emplace_back(
                                STPathElement::typeCurrency | STPathElement::typeIssuer,
                                xrpAccount(), book->getCurrencyOut(),
                                book->getIssuerOut());
                        }

                        if (hasEffectiveDestination &&
                            book->getIssuerOut() == mDstAccount &&
                            book->getCurrencyOut() == mDstAmount.getCurrency())
                        {
                        }
                        else if (book->getIssuerOut() == mEffectiveDst &&
                            book->getCurrencyOut() == mDstAmount.getCurrency())
                        { 
                            JLOG (j_.trace())
                                << "complete path found ba: "
                                << currentPath.getJson(JsonOptions::none);
                            addUniquePath (mCompletePaths, newPath);
                        }
                        else
                        {
                            incompletePaths.assembleAdd(newPath,
                                STPathElement (STPathElement::typeAccount,
                                               book->getIssuerOut (),
                                               book->getCurrencyOut (),
                                               book->getIssuerOut ()));
                        }
                    }

                }
            }
        }
    }
}

namespace {

Pathfinder::PathType makePath (char const *string)
{
    Pathfinder::PathType ret;

    while (true)
    {
        switch (*string++)
        {
            case 's': 
                ret.push_back (Pathfinder::nt_SOURCE);
                break;

            case 'a': 
                ret.push_back (Pathfinder::nt_ACCOUNTS);
                break;

            case 'b': 
                ret.push_back (Pathfinder::nt_BOOKS);
                break;

            case 'x': 
                ret.push_back (Pathfinder::nt_XRP_BOOK);
                break;

            case 'f': 
                ret.push_back (Pathfinder::nt_DEST_BOOK);
                break;

            case 'd':
                ret.push_back (Pathfinder::nt_DESTINATION);
                break;

            case 0:
                return ret;
        }
    }
}

void fillPaths (Pathfinder::PaymentType type, PathCostList const& costs)
{
    auto& list = mPathTable[type];
    assert (list.empty());
    for (auto& cost: costs)
        list.push_back ({cost.cost, makePath (cost.path)});
}

} 



void Pathfinder::initPathTable ()
{

    mPathTable.clear();
    fillPaths (pt_XRP_to_XRP, {});

    fillPaths(
        pt_XRP_to_nonXRP, {
            {1, "sfd"},   
            {3, "sfad"},  
            {5, "sfaad"}, 
            {6, "sbfd"},  
            {8, "sbafd"}, 
            {9, "sbfad"}, 
            {10, "sbafad"}
        });

    fillPaths(
        pt_nonXRP_to_XRP, {
            {1, "sxd"},       
            {2, "saxd"},      
            {6, "saaxd"},
            {7, "sbxd"},
            {8, "sabxd"},
            {9, "sabaxd"}
        });

    fillPaths(
        pt_nonXRP_to_same,  {
            {1, "sad"},     
            {1, "sfd"},     
            {4, "safd"},    
            {4, "sfad"},
            {5, "saad"},
            {5, "sbfd"},
            {6, "sxfad"},
            {6, "safad"},
            {6, "saxfd"},   
            {6, "saxfad"},
            {6, "sabfd"},   
            {7, "saaad"},
        });

    fillPaths(
        pt_nonXRP_to_nonXRP, {
            {1, "sfad"},
            {1, "safd"},
            {3, "safad"},
            {4, "sxfd"},
            {5, "saxfd"},
            {5, "sxfad"},
            {5, "sbfd"},
            {6, "saxfad"},
            {6, "sabfd"},
            {7, "saafd"},
            {8, "saafad"},
            {9, "safaad"},
        });
}

} 
























