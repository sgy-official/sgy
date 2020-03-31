

#ifndef RIPPLE_APP_LEDGER_BOOKLISTENERS_H_INCLUDED
#define RIPPLE_APP_LEDGER_BOOKLISTENERS_H_INCLUDED

#include <ripple/net/InfoSub.h>
#include <memory>
#include <mutex>

namespace ripple {


class BookListeners
{
public:
    using pointer = std::shared_ptr<BookListeners>;

    BookListeners()
    {
    }

    
    void
    addSubscriber(InfoSub::ref sub);

    
    void
    removeSubscriber(std::uint64_t sub);

    
    void
    publish(Json::Value const& jvObj, hash_set<std::uint64_t>& havePublished);

private:
    std::recursive_mutex mLock;

    hash_map<std::uint64_t, InfoSub::wptr> mListeners;
};

}  

#endif








