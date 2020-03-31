

#ifndef RIPPLE_BASICS_COUNTEDOBJECT_H_INCLUDED
#define RIPPLE_BASICS_COUNTEDOBJECT_H_INCLUDED

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace ripple {


class CountedObjects
{
public:
    static CountedObjects& getInstance () noexcept;

    using Entry = std::pair <std::string, int>;
    using List = std::vector <Entry>;

    List getCounts (int minimumThreshold) const;

public:
    
    class CounterBase
    {
    public:
        CounterBase () noexcept;

        virtual ~CounterBase () noexcept;

        int increment () noexcept
        {
            return ++m_count;
        }

        int decrement () noexcept
        {
            return --m_count;
        }

        int getCount () const noexcept
        {
            return m_count.load ();
        }

        CounterBase* getNext () const noexcept
        {
            return m_next;
        }

        virtual char const* getName () const = 0;

    private:
        virtual void checkPureVirtual () const = 0;

    protected:
        std::atomic <int> m_count;
        CounterBase* m_next;
    };

private:
    CountedObjects () noexcept;
    ~CountedObjects () noexcept = default;

private:
    std::atomic <int> m_count;
    std::atomic <CounterBase*> m_head;
};



template <class Object>
class CountedObject
{
public:
    CountedObject () noexcept
    {
        getCounter ().increment ();
    }

    CountedObject (CountedObject const&) noexcept
    {
        getCounter ().increment ();
    }

    CountedObject& operator=(CountedObject const&) noexcept = default;

    ~CountedObject () noexcept
    {
        getCounter ().decrement ();
    }

private:
    class Counter : public CountedObjects::CounterBase
    {
    public:
        Counter () noexcept { }

        char const* getName () const override
        {
            return Object::getCountedObjectName ();
        }

        void checkPureVirtual () const override { }
    };

private:
    static Counter& getCounter() noexcept
    {
        static_assert(std::is_nothrow_constructible<Counter>{}, "");
        static Counter c;
        return c;
    }
};

} 

#endif








