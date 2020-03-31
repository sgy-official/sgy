

#include <ripple/app/tx/impl/SignerEntries.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STArray.h>
#include <cstdint>

namespace ripple {

std::pair<std::vector<SignerEntries::SignerEntry>, NotTEC>
SignerEntries::deserialize (
    STObject const& obj, beast::Journal journal, std::string const& annotation)
{
    std::pair<std::vector<SignerEntry>, NotTEC> s;

    if (!obj.isFieldPresent (sfSignerEntries))
    {
        JLOG(journal.trace()) <<
            "Malformed " << annotation << ": Need signer entry array.";
        s.second = temMALFORMED;
        return s;
    }

    auto& accountVec = s.first;
    accountVec.reserve (STTx::maxMultiSigners);

    STArray const& sEntries (obj.getFieldArray (sfSignerEntries));
    for (STObject const& sEntry : sEntries)
    {
        if (sEntry.getFName () != sfSignerEntry)
        {
            JLOG(journal.trace()) <<
                "Malformed " << annotation << ": Expected SignerEntry.";
            s.second = temMALFORMED;
            return s;
        }

        AccountID const account = sEntry.getAccountID (sfAccount);
        std::uint16_t const weight = sEntry.getFieldU16 (sfSignerWeight);
        accountVec.emplace_back (account, weight);
    }

    s.second = tesSUCCESS;
    return s;
}

} 
























