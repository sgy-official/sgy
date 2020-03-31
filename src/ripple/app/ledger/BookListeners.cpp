

#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/json/to_string.h>

namespace ripple {

void
BookListeners::addSubscriber(InfoSub::ref sub)
{
    std::lock_guard<std::recursive_mutex> sl(mLock);
    mListeners[sub->getSeq()] = sub;
}

void
BookListeners::removeSubscriber(std::uint64_t seq)
{
    std::lock_guard<std::recursive_mutex> sl(mLock);
    mListeners.erase(seq);
}

void
BookListeners::publish(
    Json::Value const& jvObj,
    hash_set<std::uint64_t>& havePublished)
{
    std::lock_guard<std::recursive_mutex> sl(mLock);
    auto it = mListeners.cbegin();

    while (it != mListeners.cend())
    {
        InfoSub::pointer p = it->second.lock();

        if (p)
        {
            if(havePublished.emplace(p->getSeq()).second)
            {
                p->send(jvObj, true);
            }
            ++it;
        }
        else
            it = mListeners.erase(it);
    }
}

}  
























