

#ifndef RIPPLE_LEDGER_RAWVIEW_H_INCLUDED
#define RIPPLE_LEDGER_RAWVIEW_H_INCLUDED

#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <memory>
#include <utility>

namespace ripple {


class RawView
{
public:
    virtual ~RawView() = default;
    RawView() = default;
    RawView(RawView const&) = default;
    RawView& operator=(RawView const&) = delete;

    
    virtual
    void
    rawErase (std::shared_ptr<SLE> const& sle) = 0;

    
    virtual
    void
    rawInsert (std::shared_ptr<SLE> const& sle) = 0;

    
    virtual
    void
    rawReplace (std::shared_ptr<SLE> const& sle) = 0;

    
    virtual
    void
    rawDestroyXRP (XRPAmount const& fee) = 0;
};



class TxsRawView : public RawView
{
public:
    
    virtual
    void
    rawTxInsert (ReadView::key_type const& key,
        std::shared_ptr<Serializer const>
            const& txn, std::shared_ptr<
                Serializer const> const& metaData) = 0;
};

} 

#endif








