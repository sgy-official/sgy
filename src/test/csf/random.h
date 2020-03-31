

#ifndef RIPPLE_TEST_CSF_RANDOM_H_INCLUDED
#define RIPPLE_TEST_CSF_RANDOM_H_INCLUDED

#include <random>
#include <vector>

namespace ripple {
namespace test {
namespace csf {


template <class T, class G>
std::vector<T>
random_weighted_shuffle(std::vector<T> v, std::vector<double> w, G& g)
{
    using std::swap;

    for (int i = 0; i < v.size() - 1; ++i)
    {
        std::discrete_distribution<> dd(w.begin() + i, w.end());
        auto idx = dd(g);
        std::swap(v[i], v[idx]);
        std::swap(w[i], w[idx]);
    }
    return v;
}


template <class RandomNumberDistribution, class Generator>
std::vector<typename RandomNumberDistribution::result_type>
sample( std::size_t size, RandomNumberDistribution dist, Generator& g)
{
    std::vector<typename RandomNumberDistribution::result_type> res(size);
    std::generate(res.begin(), res.end(), [&dist, &g]() { return dist(g); });
    return res;
}


template <class RAIter, class Generator>
class Selector
{
    RAIter first_, last_;
    std::discrete_distribution<> dd_;
    Generator g_;

public:
    
    Selector(RAIter first, RAIter last, std::vector<double> const& w,
            Generator& g)
      : first_{first}, last_{last}, dd_{w.begin(), w.end()}, g_{g}
    {
        using tag = typename std::iterator_traits<RAIter>::iterator_category;
        static_assert(
                std::is_same<tag, std::random_access_iterator_tag>::value,
                "Selector only supports random access iterators.");
    }

    typename std::iterator_traits<RAIter>::value_type
    operator()()
    {
        auto idx = dd_(g_);
        return *(first_ + idx);
    }
};

template <typename Iter, typename Generator>
Selector<Iter,Generator>
makeSelector(Iter first, Iter last, std::vector<double> const& w, Generator& g)
{
    return Selector<Iter, Generator>(first, last, w, g);
}



class ConstantDistribution
{
    double t_;

public:
    ConstantDistribution(double const& t) : t_{t}
    {
    }

    template <class Generator>
    inline double
    operator()(Generator& )
    {
        return t_;
    }
};


class PowerLawDistribution
{
    double xmin_;
    double a_;
    double inv_;
    std::uniform_real_distribution<double> uf_{0, 1};

public:

    using result_type = double;

    PowerLawDistribution(double xmin, double a) : xmin_{xmin}, a_{a}
    {
        inv_ = 1.0 / (1.0 - a_);
    }

    template <class Generator>
    inline double
    operator()(Generator& g)
    {
        return xmin_ * std::pow(1 - uf_(g), inv_);
    }
};

}  
}  
}  

#endif








