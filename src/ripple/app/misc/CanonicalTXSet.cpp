

#include <ripple/app/misc/CanonicalTXSet.h>
#include <boost/range/adaptor/transformed.hpp>

namespace ripple {

bool CanonicalTXSet::Key::operator< (Key const& rhs) const
{
    if (mAccount < rhs.mAccount) return true;

    if (mAccount > rhs.mAccount) return false;

    if (mSeq < rhs.mSeq) return true;

    if (mSeq > rhs.mSeq) return false;

    return mTXid < rhs.mTXid;
}

bool CanonicalTXSet::Key::operator> (Key const& rhs) const
{
    if (mAccount > rhs.mAccount) return true;

    if (mAccount < rhs.mAccount) return false;

    if (mSeq > rhs.mSeq) return true;

    if (mSeq < rhs.mSeq) return false;

    return mTXid > rhs.mTXid;
}

bool CanonicalTXSet::Key::operator<= (Key const& rhs) const
{
    if (mAccount < rhs.mAccount) return true;

    if (mAccount > rhs.mAccount) return false;

    if (mSeq < rhs.mSeq) return true;

    if (mSeq > rhs.mSeq) return false;

    return mTXid <= rhs.mTXid;
}

bool CanonicalTXSet::Key::operator>= (Key const& rhs)const
{
    if (mAccount > rhs.mAccount) return true;

    if (mAccount < rhs.mAccount) return false;

    if (mSeq > rhs.mSeq) return true;

    if (mSeq < rhs.mSeq) return false;

    return mTXid >= rhs.mTXid;
}

uint256 CanonicalTXSet::accountKey (AccountID const& account)
{
    uint256 ret = beast::zero;
    memcpy (
        ret.begin (),
        account.begin (),
        account.size ());
    ret ^= salt_;
    return ret;
}

void CanonicalTXSet::insert (std::shared_ptr<STTx const> const& txn)
{
    map_.insert (
        std::make_pair (
            Key (
                accountKey (txn->getAccountID(sfAccount)),
                txn->getSequence (),
                txn->getTransactionID ()),
            txn));
}

std::vector<std::shared_ptr<STTx const>>
CanonicalTXSet::prune(AccountID const& account,
    std::uint32_t const seq)
{
    auto effectiveAccount = accountKey (account);

    Key keyLow(effectiveAccount, seq, beast::zero);
    Key keyHigh(effectiveAccount, seq+1, beast::zero);

    auto range = boost::make_iterator_range(
        map_.lower_bound(keyLow),
        map_.lower_bound(keyHigh));
    auto txRange = boost::adaptors::transform(range,
        [](auto const& p) { return p.second; });

    std::vector<std::shared_ptr<STTx const>> result(
        txRange.begin(), txRange.end());

    map_.erase(range.begin(), range.end());
    return result;
}

} 
























