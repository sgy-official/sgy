
#ifndef RIPPLE_TEST_CSF_COLLECTOREF_H_INCLUDED
#define RIPPLE_TEST_CSF_COLLECTOREF_H_INCLUDED

#include <test/csf/events.h>
#include <test/csf/SimTime.h>

namespace ripple {
namespace test {
namespace csf {


class CollectorRef
{
    using tp = SimTime;

    struct ICollector
    {
        virtual ~ICollector() = default;

        virtual void
        on(PeerID node, tp when, Share<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, SubmitTx const&) = 0;

        virtual void
        on(PeerID node, tp when, StartRound const&) = 0;

        virtual void
        on(PeerID node, tp when, CloseLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, AcceptLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, WrongPrevLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, FullyValidateLedger const&) = 0;
    };

    template <class T>
    class Any final : public ICollector
    {
        T & t_;

    public:
        Any(T & t) : t_{t}
        {
        }

        Any(Any const & ) = delete;
        Any& operator=(Any const & ) = delete;

        Any(Any && ) = default;
        Any& operator=(Any && ) = default;

        virtual void
        on(PeerID node, tp when, Share<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        void
        on(PeerID node, tp when, Receive<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        void
        on(PeerID node, tp when, Relay<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, SubmitTx const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, StartRound const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, CloseLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, AcceptLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, WrongPrevLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, FullyValidateLedger const& e) override
        {
            t_.on(node, when, e);
        }
    };

    std::unique_ptr<ICollector> impl_;

public:
    template <class T>
    CollectorRef(T& t) : impl_{new Any<T>(t)}
    {
    }

    CollectorRef(CollectorRef const& c) = delete;
    CollectorRef& operator=(CollectorRef& c) = delete;

    CollectorRef(CollectorRef&&) = default;
    CollectorRef& operator=(CollectorRef&&) = default;

    template <class E>
    void
    on(PeerID node, tp when, E const& e)
    {
        impl_->on(node, when, e);
    }
};


class CollectorRefs
{
    std::vector<CollectorRef> collectors_;
public:

    template <class Collector>
    void add(Collector & collector)
    {
        collectors_.emplace_back(collector);
    }

    template <class E>
    void
    on(PeerID node, SimTime when, E const& e)
    {
        for (auto & c : collectors_)
        {
            c.on(node, when, e);
        }
    }

};

}  
}  
}  

#endif








