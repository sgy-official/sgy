

#ifndef RIPPLE_PROTOCOL_PROTOCOL_H_INCLUDED
#define RIPPLE_PROTOCOL_PROTOCOL_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/ByteUtilities.h>
#include <cstdint>

namespace ripple {



std::size_t constexpr txMinSizeBytes = 32;


std::size_t constexpr txMaxSizeBytes = megabytes(1);


std::size_t constexpr unfundedOfferRemoveLimit = 1000;


std::size_t constexpr oversizeMetaDataCap = 5200;


std::size_t constexpr dirNodeMaxEntries = 32;


std::uint64_t constexpr dirNodeMaxPages = 262144;


using LedgerIndex = std::uint32_t;


using TxID = uint256;

using TxSeq = std::uint32_t;

} 

#endif








