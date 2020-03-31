

#ifndef RIPPLE_BASICS_LOCALVALUE_H_INCLUDED
#define RIPPLE_BASICS_LOCALVALUE_H_INCLUDED

#include <boost/thread/tss.hpp>
#include <memory>
#include <unordered_map>

namespace ripple {

namespace detail {

struct LocalValues
{
    explicit LocalValues() = default;

    bool onCoro = true;

    struct BasicValue
    {
        virtual ~BasicValue() = default;
        virtual void* get() = 0;
    };

    template <class T>
    struct Value : BasicValue
    {
        T t_;

        Value() = default;
        explicit Value(T const& t) : t_(t) {}

        void* get() override
        {
            return &t_;
        }
    };

    std::unordered_map<void const*, std::unique_ptr<BasicValue>> values;

    static
    inline
    void
    cleanup(LocalValues* lvs)
    {
        if (lvs && ! lvs->onCoro)
            delete lvs;
    }
};

template<class = void>
boost::thread_specific_ptr<detail::LocalValues>&
getLocalValues()
{
    static boost::thread_specific_ptr<
        detail::LocalValues> tsp(&detail::LocalValues::cleanup);
    return tsp;
}

} 

template <class T>
class LocalValue
{
public:
    template <class... Args>
    LocalValue(Args&&... args)
        : t_(std::forward<Args>(args)...)
    {
    }

    
    T& operator*();

    
    T* operator->()
    {
        return &**this;
    }

private:
    T t_;
};

template <class T>
T&
LocalValue<T>::operator*()
{
    auto lvs = detail::getLocalValues().get();
    if (! lvs)
    {
        lvs = new detail::LocalValues();
        lvs->onCoro = false;
        detail::getLocalValues().reset(lvs);
    }
    else
    {
        auto const iter = lvs->values.find(this);
        if (iter != lvs->values.end())
            return *reinterpret_cast<T*>(iter->second->get());
    }

    return *reinterpret_cast<T*>(lvs->values.emplace(this,
        std::make_unique<detail::LocalValues::Value<T>>(t_)).
            first->second->get());
}
} 

#endif








