

#ifndef RIPPLE_PEERFINDER_SOURCE_H_INCLUDED
#define RIPPLE_PEERFINDER_SOURCE_H_INCLUDED

#include <ripple/peerfinder/PeerfinderManager.h>
#include <boost/system/error_code.hpp>

namespace ripple {
namespace PeerFinder {


class Source
{
public:
    
    struct Results
    {
        explicit Results() = default;

        boost::system::error_code error;

        IPAddresses addresses;
    };

    virtual ~Source () { }
    virtual std::string const& name () = 0;
    virtual void cancel () { }
    virtual void fetch (Results& results, beast::Journal journal) = 0;
};

}
}

#endif








