

#ifndef RIPPLE_TX_IMPL_SIGNER_ENTRIES_H_INCLUDED
#define RIPPLE_TX_IMPL_SIGNER_ENTRIES_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>    
#include <ripple/protocol/STTx.h>             
#include <ripple/protocol/UintTypes.h>        
#include <ripple/protocol/TER.h>              
#include <ripple/beast/utility/Journal.h>     

namespace ripple {

class STObject;

class SignerEntries
{
public:
    explicit SignerEntries() = default;

    struct SignerEntry
    {
        AccountID account;
        std::uint16_t weight;

        SignerEntry (AccountID const& inAccount, std::uint16_t inWeight)
        : account (inAccount)
        , weight (inWeight)
        { }

        friend bool operator< (SignerEntry const& lhs, SignerEntry const& rhs)
        {
            return lhs.account < rhs.account;
        }

        friend bool operator== (SignerEntry const& lhs, SignerEntry const& rhs)
        {
            return lhs.account == rhs.account;
        }
    };

    static
    std::pair<std::vector<SignerEntry>, NotTEC>
    deserialize (
        STObject const& obj,
        beast::Journal journal,
        std::string const& annotation);
};

} 

#endif 








