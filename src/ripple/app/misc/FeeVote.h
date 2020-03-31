

#ifndef RIPPLE_APP_MISC_FEEVOTE_H_INCLUDED
#define RIPPLE_APP_MISC_FEEVOTE_H_INCLUDED

#include <ripple/ledger/ReadView.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/protocol/SystemParameters.h>

namespace ripple {


class FeeVote
{
public:
    
    struct Setup
    {
        
        std::uint64_t reference_fee = 10;

        
        std::uint32_t const reference_fee_units = 10;

        
        std::uint64_t account_reserve = 0 * SYSTEM_CURRENCY_PARTS;

        
        std::uint64_t owner_reserve = 0 * SYSTEM_CURRENCY_PARTS;
    };

    virtual ~FeeVote () = default;

    
    virtual
    void
    doValidation (std::shared_ptr<ReadView const> const& lastClosedLedger,
        STValidation::FeeSettings & fees) = 0;

    
    virtual
    void
    doVoting (std::shared_ptr<ReadView const> const& lastClosedLedger,
        std::vector<STValidation::pointer> const& parentValidations,
            std::shared_ptr<SHAMap> const& initialPosition) = 0;
};


FeeVote::Setup
setup_FeeVote (Section const& section);


std::unique_ptr <FeeVote>
make_FeeVote (FeeVote::Setup const& setup, beast::Journal journal);

} 

#endif








