

#ifndef RIPPLE_APP_LEDGER_ABSTRACTFETCHPACKCONTAINER_H_INCLUDED
#define RIPPLE_APP_LEDGER_ABSTRACTFETCHPACKCONTAINER_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Blob.h>
#include <boost/optional.hpp>

namespace ripple {


class AbstractFetchPackContainer
{
public:
    virtual ~AbstractFetchPackContainer() = default;

    
    virtual boost::optional<Blob> getFetchPack(uint256 const& nodeHash) = 0;
};

} 

#endif








