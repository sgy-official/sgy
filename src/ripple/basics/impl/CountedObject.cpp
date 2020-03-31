

#include <ripple/basics/CountedObject.h>
#include <type_traits>

namespace ripple {

CountedObjects& CountedObjects::getInstance () noexcept
{
    static CountedObjects instance;

    return instance;
}

CountedObjects::CountedObjects () noexcept
    : m_count (0)
    , m_head (nullptr)
{
}

CountedObjects::List CountedObjects::getCounts (int minimumThreshold) const
{
    List counts;

    int const count = m_count.load ();

    counts.reserve (count);

    CounterBase* counter = m_head.load ();

    while (counter != nullptr)
    {
        if (counter->getCount () >= minimumThreshold)
        {
            Entry entry;

            entry.first = counter->getName ();
            entry.second = counter->getCount ();

            counts.push_back (entry);
        }

        counter = counter->getNext ();
    }

    return counts;
}


CountedObjects::CounterBase::CounterBase () noexcept
    : m_count (0)
{

    CountedObjects& instance = CountedObjects::getInstance ();
    CounterBase* head;

    do
    {
        head = instance.m_head.load ();
        m_next = head;
    }
    while (instance.m_head.exchange (this) != head);

    ++instance.m_count;
}

CountedObjects::CounterBase::~CounterBase () noexcept
{
}

} 
























