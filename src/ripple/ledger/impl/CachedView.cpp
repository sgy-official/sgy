

#include <ripple/ledger/CachedView.h>
#include <ripple/basics/contract.h>
#include <ripple/protocol/Serializer.h>

namespace ripple {
namespace detail {

bool
CachedViewImpl::exists (Keylet const& k) const
{
    return read(k) != nullptr;
}

std::shared_ptr<SLE const>
CachedViewImpl::read(Keylet const& k) const
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto const iter = map_.find(k.key);
        if (iter != map_.end())
        {
            if (!iter->second || !k.check(*iter->second))
                return nullptr;
            return iter->second;
        }
    }
    auto const digest = base_.digest(k.key);
    if (!digest)
        return nullptr;
    auto sle = cache_.fetch(*digest, [&]() { return base_.read(k); });
    std::lock_guard<std::mutex> lock(mutex_);
    auto const er = map_.emplace(k.key, sle);
    auto const& iter = er.first;
    bool const inserted = er.second;
    if (iter->second && !k.check(*iter->second))
    {
        if (!inserted)
        {
            LogicError("CachedView::read: wrong type");
        }
        return nullptr;
    }
    return iter->second;
}

} 
} 
























