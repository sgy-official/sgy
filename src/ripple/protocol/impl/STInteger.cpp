

#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/TxFormats.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/core/LexicalCast.h>

namespace ripple {

template<>
STInteger<unsigned char>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get8())
{
}

template <>
SerializedTypeID
STUInt8::getSType () const
{
    return STI_UINT8;
}

template <>
std::string
STUInt8::getText () const
{
    if (getFName () == sfTransactionResult)
    {
        std::string token, human;

        if (transResultInfo (TER::fromInt (value_), token, human))
            return human;

        JLOG (debugLog().error())
            << "Unknown result code in metadata: " << value_;
    }

    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt8::getJson (JsonOptions) const
{
    if (getFName () == sfTransactionResult)
    {
        std::string token, human;

        if (transResultInfo (TER::fromInt (value_), token, human))
            return token;

        JLOG (debugLog().error())
            << "Unknown result code in metadata: " << value_;
    }

    return value_;
}


template<>
STInteger<std::uint16_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get16())
{
}

template <>
SerializedTypeID
STUInt16::getSType () const
{
    return STI_UINT16;
}

template <>
std::string
STUInt16::getText () const
{
    if (getFName () == sfLedgerEntryType)
    {
        auto item = LedgerFormats::getInstance ().findByType (
            safe_cast<LedgerEntryType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    if (getFName () == sfTransactionType)
    {
        auto item =TxFormats::getInstance().findByType (
            safe_cast<TxType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt16::getJson (JsonOptions) const
{
    if (getFName () == sfLedgerEntryType)
    {
        auto item = LedgerFormats::getInstance ().findByType (
            safe_cast<LedgerEntryType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    if (getFName () == sfTransactionType)
    {
        auto item = TxFormats::getInstance().findByType (
            safe_cast<TxType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    return value_;
}


template<>
STInteger<std::uint32_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get32())
{
}

template <>
SerializedTypeID
STUInt32::getSType () const
{
    return STI_UINT32;
}

template <>
std::string
STUInt32::getText () const
{
    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt32::getJson (JsonOptions) const
{
    return value_;
}


template<>
STInteger<std::uint64_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get64())
{
}

template <>
SerializedTypeID
STUInt64::getSType () const
{
    return STI_UINT64;
}

template <>
std::string
STUInt64::getText () const
{
    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt64::getJson (JsonOptions) const
{
    return strHex (value_);
}

} 
























