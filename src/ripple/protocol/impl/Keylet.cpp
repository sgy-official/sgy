

#include <ripple/protocol/Keylet.h>
#include <ripple/protocol/STLedgerEntry.h>

namespace ripple {

bool
Keylet::check (SLE const& sle) const
{
    if (type == ltANY)
        return true;
    if (type == ltINVALID)
        return false;
    if (type == ltCHILD)
    {
        assert(sle.getType() != ltDIR_NODE);
        return sle.getType() != ltDIR_NODE;
    }
    assert(sle.getType() == type);
    return sle.getType() == type;
}

} 
























