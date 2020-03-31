
#ifndef RIPPLE_TEST_CSF_SUBMITTERS_H_INCLUDED
#define RIPPLE_TEST_CSF_SUBMITTERS_H_INCLUDED

#include <test/csf/SimTime.h>
#include <test/csf/Scheduler.h>
#include <test/csf/Peer.h>
#include <test/csf/Tx.h>
#include <type_traits>
namespace ripple {
namespace test {
namespace csf {



struct Rate
{
    std::size_t count;
    SimDuration duration;

    double
    inv() const
    {
        return duration.count()/double(count);
    }
};


template <class Distribution, class Generator, class Selector>
class Submitter
{
    Distribution dist_;
    SimTime stop_;
    std::uint32_t nextID_ = 0;
    Selector selector_;
    Scheduler & scheduler_;
    Generator & g_;

    static SimDuration
    asDuration(SimDuration d)
    {
        return d;
    }

    template <class T>
    static
    std::enable_if_t<std::is_arithmetic<T>::value, SimDuration>
    asDuration(T t)
    {
        return SimDuration{static_cast<SimDuration::rep>(t)};
    }

    void
    submit()
    {
        selector_()->submit(Tx{nextID_++});
        if (scheduler_.now() < stop_)
        {
            scheduler_.in(asDuration(dist_(g_)), [&]() { submit(); });
        }
    }

public:
    Submitter(
        Distribution dist,
        SimTime start,
        SimTime end,
        Selector & selector,
        Scheduler & s,
        Generator & g)
        : dist_{dist}, stop_{end}, selector_{selector}, scheduler_{s}, g_{g}
    {
        scheduler_.at(start, [&]() { submit(); });
    }
};

template <class Distribution, class Generator, class Selector>
Submitter<Distribution, Generator, Selector>
makeSubmitter(
    Distribution dist,
    SimTime start,
    SimTime end,
    Selector& sel,
    Scheduler& s,
    Generator& g)
{
    return Submitter<Distribution, Generator, Selector>(
            dist, start ,end, sel, s, g);
}

}  
}  
}  

#endif








