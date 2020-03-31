

#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/ledger/OpenView.h>

namespace ripple {

RippleLineCache::RippleLineCache(
    std::shared_ptr <ReadView const> const& ledger)
{
    mLedger = std::make_shared<OpenView>(&*ledger, ledger);
}

std::vector<RippleState::pointer> const&
RippleLineCache::getRippleLines (AccountID const& accountID)
{
    AccountKey key (accountID, hasher_ (accountID));

    std::lock_guard <std::mutex> sl (mLock);

    auto it = lines_.emplace (key,
        std::vector<RippleState::pointer>());

    if (it.second)
        it.first->second = getRippleStateItems (
            accountID, *mLedger);

    return it.first->second;
}

} 
























