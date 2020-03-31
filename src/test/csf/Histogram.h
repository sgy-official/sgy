
#ifndef RIPPLE_TEST_CSF_HISTOGRAM_H_INCLUDED
#define RIPPLE_TEST_CSF_HISTOGRAM_H_INCLUDED

#include <map>
#include <chrono>
#include <algorithm>

namespace ripple {
namespace test {
namespace csf {


template <class T, class Compare = std::less<T>>
class Histogram
{
    std::map<T, std::size_t, Compare> counts_;
    std::size_t samples = 0;
public:
    
    void
    insert(T const & s)
    {
        ++counts_[s];
        ++samples;
    }

    
    std::size_t
    size() const
    {
        return samples;
    }

    
    std::size_t
    numBins() const
    {
        return counts_.size();
    }

    
    T
    minValue() const
    {
        return counts_.empty() ? T{} : counts_.begin()->first;
    }

    
    T
    maxValue() const
    {
        return counts_.empty() ? T{} : counts_.rbegin()->first;
    }

    
    T
    avg() const
    {
        T tmp{};
        if(samples == 0)
            return tmp;
        for (auto const& it : counts_)
        {
            tmp += it.first * it.second;
        }
        return tmp/samples;
    }

    
    T
    percentile(float p) const
    {
        assert(p >= 0 && p <=1);
        std::size_t pos = std::round(p * samples);

        if(counts_.empty())
            return T{};

        auto it = counts_.begin();
        std::size_t cumsum = it->second;
        while (it != counts_.end() && cumsum < pos)
        {
            ++it;
            cumsum += it->second;
        }
        return it->first;
    }
};

}  
}  
}  

#endif








