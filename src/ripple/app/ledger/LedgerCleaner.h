

#ifndef RIPPLE_APP_LEDGER_LEDGERCLEANER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERCLEANER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/json/json_value.h>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>

namespace ripple {
namespace detail {


class LedgerCleaner
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
    explicit LedgerCleaner (Stoppable& parent);

public:
    
    virtual ~LedgerCleaner () = 0;

    
    virtual void doClean (Json::Value const& parameters) = 0;
};

std::unique_ptr<LedgerCleaner>
make_LedgerCleaner (Application& app,
    Stoppable& parent, beast::Journal journal);

} 
} 

#endif








