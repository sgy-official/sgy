

#ifndef RIPPLE_PEERFINDER_SIM_FUNCTIONQUEUE_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_FUNCTIONQUEUE_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {


class FunctionQueue
{
public:
    explicit FunctionQueue() = default;

private:
    class BasicWork
    {
    public:
        virtual ~BasicWork ()
            { }
        virtual void operator() () = 0;
    };

    template <typename Function>
    class Work : public BasicWork
    {
    public:
        explicit Work (Function f)
            : m_f (f)
            { }
        void operator() ()
            { (m_f)(); }
    private:
        Function m_f;
    };

    std::list <std::unique_ptr <BasicWork>> m_work;

public:
    
    bool empty ()
        { return m_work.empty(); }

    
    template <typename Function>
    void post (Function f)
    {
        m_work.emplace_back (std::make_unique<Work <Function>>(f));
    }

    
    void run ()
    {
        while (! m_work.empty ())
        {
            (*m_work.front())();
            m_work.pop_front();
        }
    }
};

}
}
}

#endif








