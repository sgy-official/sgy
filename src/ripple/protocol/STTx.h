

#ifndef RIPPLE_PROTOCOL_STTX_H_INCLUDED
#define RIPPLE_PROTOCOL_STTX_H_INCLUDED

#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/TxFormats.h>
#include <boost/container/flat_set.hpp>
#include <boost/logic/tribool.hpp>
#include <functional>

namespace ripple {

enum TxnSql : char
{
    txnSqlNew = 'N',
    txnSqlConflict = 'C',
    txnSqlHeld = 'H',
    txnSqlValidated = 'V',
    txnSqlIncluded = 'I',
    txnSqlUnknown = 'U'
};

class STTx final
    : public STObject
    , public CountedObject <STTx>
{
public:
    static char const* getCountedObjectName () { return "STTx"; }

    static std::size_t const minMultiSigners = 1;
    static std::size_t const maxMultiSigners = 8;

public:
    STTx () = delete;
    STTx& operator= (STTx const& other) = delete;

    STTx (STTx const& other) = default;

    explicit STTx (SerialIter& sit) noexcept (false);
    explicit STTx (SerialIter&& sit) noexcept (false) : STTx(sit) {}

    explicit STTx (STObject&& object) noexcept (false);

    
    STTx (
        TxType type,
        std::function<void(STObject&)> assembler);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    SerializedTypeID getSType () const override
    {
        return STI_TRANSACTION;
    }
    std::string getFullText () const override;

    Blob getSignature () const;

    uint256 getSigningHash () const;

    TxType getTxnType () const
    {
        return tx_type_;
    }

    Blob getSigningPubKey () const
    {
        return getFieldVL (sfSigningPubKey);
    }

    std::uint32_t getSequence () const
    {
        return getFieldU32 (sfSequence);
    }
    void setSequence (std::uint32_t seq)
    {
        return setFieldU32 (sfSequence, seq);
    }

    boost::container::flat_set<AccountID>
    getMentionedAccounts() const;

    uint256 getTransactionID () const
    {
        return tid_;
    }

    Json::Value getJson (JsonOptions options) const override;
    Json::Value getJson (JsonOptions options, bool binary) const;

    void sign (
        PublicKey const& publicKey,
        SecretKey const& secretKey);

    
    std::pair<bool, std::string>
    checkSign(bool allowMultiSign) const;

    static
    std::string const&
    getMetaSQLInsertReplaceHeader ();

    std::string getMetaSQL (
        std::uint32_t inLedger, std::string const& escapedMetaData) const;

    std::string getMetaSQL (
        Serializer rawTxn,
        std::uint32_t inLedger,
        char status,
        std::string const& escapedMetaData) const;

private:
    std::pair<bool, std::string> checkSingleSign () const;
    std::pair<bool, std::string> checkMultiSign () const;

    uint256 tid_;
    TxType tx_type_;
};

bool passesLocalChecks (STObject const& st, std::string&);


std::shared_ptr<STTx const>
sterilize (STTx const& stx);


bool isPseudoTx(STObject const& tx);

} 

#endif








