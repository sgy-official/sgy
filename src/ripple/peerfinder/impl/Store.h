

#ifndef RIPPLE_PEERFINDER_STORE_H_INCLUDED
#define RIPPLE_PEERFINDER_STORE_H_INCLUDED

namespace ripple {
namespace PeerFinder {


class Store
{
public:
    virtual ~Store () { }

    using load_callback = std::function <void (beast::IP::Endpoint, int)>;
    virtual std::size_t load (load_callback const& cb) = 0;

    struct Entry
    {
        explicit Entry() = default;

        beast::IP::Endpoint endpoint;
        int valence;
    };
    virtual void save (std::vector <Entry> const& v) = 0;
};

}
}

#endif








