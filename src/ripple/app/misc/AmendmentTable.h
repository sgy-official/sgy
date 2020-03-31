

#ifndef RIPPLE_APP_MISC_AMENDMENTTABLE_H_INCLUDED
#define RIPPLE_APP_MISC_AMENDMENTTABLE_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/Protocol.h>

namespace ripple {


class AmendmentTable
{
public:
    virtual ~AmendmentTable() = default;

    virtual uint256 find (std::string const& name) = 0;

    virtual bool veto (uint256 const& amendment) = 0;
    virtual bool unVeto (uint256 const& amendment) = 0;

    virtual bool enable (uint256 const& amendment) = 0;
    virtual bool disable (uint256 const& amendment) = 0;

    virtual bool isEnabled (uint256 const& amendment) = 0;
    virtual bool isSupported (uint256 const& amendment) = 0;

    
    virtual bool hasUnsupportedEnabled () = 0;

    virtual Json::Value getJson (int) = 0;

    
    virtual Json::Value getJson (uint256 const& ) = 0;

    
    void doValidatedLedger (std::shared_ptr<ReadView const> const& lastValidatedLedger)
    {
        if (needValidatedLedger (lastValidatedLedger->seq ()))
            doValidatedLedger (lastValidatedLedger->seq (),
                getEnabledAmendments (*lastValidatedLedger));
    }

    
    virtual bool
    needValidatedLedger (LedgerIndex seq) = 0;

    virtual void
    doValidatedLedger (
        LedgerIndex ledgerSeq,
        std::set <uint256> const& enabled) = 0;

    virtual std::map <uint256, std::uint32_t>
    doVoting (
        NetClock::time_point closeTime,
        std::set <uint256> const& enabledAmendments,
        majorityAmendments_t const& majorityAmendments,
        std::vector<STValidation::pointer> const& valSet) = 0;

    virtual std::vector <uint256>
    doValidation (std::set <uint256> const& enabled) = 0;

    virtual std::vector <uint256>
    getDesired () = 0;


    void
    doVoting (
        std::shared_ptr <ReadView const> const& lastClosedLedger,
        std::vector<STValidation::pointer> const& parentValidations,
        std::shared_ptr<SHAMap> const& initialPosition)
    {
        auto actions = doVoting (
            lastClosedLedger->parentCloseTime(),
            getEnabledAmendments(*lastClosedLedger),
            getMajorityAmendments(*lastClosedLedger),
            parentValidations);

        for (auto const& it : actions)
        {
            STTx amendTx (ttAMENDMENT,
                [&it, seq = lastClosedLedger->seq() + 1](auto& obj)
                {
                    obj.setAccountID (sfAccount, AccountID());
                    obj.setFieldH256 (sfAmendment, it.first);
                    obj.setFieldU32 (sfLedgerSequence, seq);

                    if (it.second != 0)
                        obj.setFieldU32 (sfFlags, it.second);
                });

            Serializer s;
            amendTx.add (s);

            initialPosition->addGiveItem (
                std::make_shared <SHAMapItem> (
                    amendTx.getTransactionID(),
                    s.peekData()),
                true,
                false);
        }
    }

};

std::unique_ptr<AmendmentTable> make_AmendmentTable (
    std::chrono::seconds majorityTime,
    int majorityFraction,
    Section const& supported,
    Section const& enabled,
    Section const& vetoed,
    beast::Journal journal);

}  

#endif








