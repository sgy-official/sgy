

#ifndef RIPPLE_APP_PENDINGSAVES_H_INCLUDED
#define RIPPLE_APP_PENDINGSAVES_H_INCLUDED

#include <ripple/protocol/Protocol.h>
#include <map>
#include <mutex>
#include <condition_variable>

namespace ripple {


class PendingSaves
{
private:
    std::mutex mutable mutex_;
    std::map <LedgerIndex, bool> map_;
    std::condition_variable await_;

public:

    
    bool
    startWork (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);

        auto it = map_.find (seq);

        if ((it == map_.end()) || it->second)
        {
            return false;
        }

        it->second = true;
        return true;
    }

    
    void
    finishWork (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);

        map_.erase (seq);
        await_.notify_all();
    }

    
    bool
    pending (LedgerIndex seq)
    {
        std::lock_guard <std::mutex> lock(mutex_);
        return map_.find(seq) != map_.end();
    }

    
    bool
    shouldWork (LedgerIndex seq, bool isSynchronous)
    {
        std::unique_lock <std::mutex> lock(mutex_);
        do
        {
            auto it = map_.find (seq);

            if (it == map_.end())
            {
                map_.emplace(seq, false);
                return true;
            }

            if (! isSynchronous)
            {
                return false;
            }

            if (! it->second)
            {
                return true;
            }

            await_.wait (lock);

        } while (true);
    }

    
    std::map <LedgerIndex, bool>
    getSnapshot () const
    {
        std::lock_guard <std::mutex> lock(mutex_);

        return map_;
    }

};

}

#endif








