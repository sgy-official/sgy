

#ifndef RIPPLE_PROTOCOL_HASHPREFIX_H_INCLUDED
#define RIPPLE_PROTOCOL_HASHPREFIX_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <cstdint>

namespace ripple {


class HashPrefix
{
private:
    std::uint32_t m_prefix;

    HashPrefix (char a, char b, char c)
        : m_prefix (0)
    {
        m_prefix = a;
        m_prefix = (m_prefix << 8) + b;
        m_prefix = (m_prefix << 8) + c;
        m_prefix = m_prefix << 8;
    }

public:
    HashPrefix(HashPrefix const&) = delete;
    HashPrefix& operator=(HashPrefix const&) = delete;

    
    operator std::uint32_t () const
    {
        return m_prefix;
    }


    
    static HashPrefix const transactionID;

    
    static HashPrefix const txNode;

    
    static HashPrefix const leafNode;

    
    static HashPrefix const innerNode;

    
    static HashPrefix const innerNodeV2;

    
    static HashPrefix const ledgerMaster;

    
    static HashPrefix const txSign;

    
    static HashPrefix const txMultiSign;

    
    static HashPrefix const validation;

    
    static HashPrefix const proposal;

    
    static HashPrefix const manifest;

    
    static HashPrefix const paymentChannelClaim;
};

template <class Hasher>
void
hash_append (Hasher& h, HashPrefix const& hp) noexcept
{
    using beast::hash_append;
    hash_append(h,
        static_cast<std::uint32_t>(hp));
}

} 

#endif








