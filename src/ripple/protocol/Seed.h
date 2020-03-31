

#ifndef RIPPLE_PROTOCOL_SEED_H_INCLUDED
#define RIPPLE_PROTOCOL_SEED_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/tokens.h>
#include <boost/optional.hpp>
#include <array>

namespace ripple {


class Seed
{
private:
    std::array<uint8_t, 16> buf_;

public:
    using const_iterator = std::array<uint8_t, 16>::const_iterator;

    Seed() = delete;

    Seed (Seed const&) = default;
    Seed& operator= (Seed const&) = default;

    
    ~Seed();

    
    
    explicit Seed (Slice const& slice);
    explicit Seed (uint128 const& seed);
    

    std::uint8_t const*
    data() const
    {
        return buf_.data();
    }

    std::size_t
    size() const
    {
        return buf_.size();
    }

    const_iterator
    begin() const noexcept
    {
        return buf_.begin();
    }

    const_iterator
    cbegin() const noexcept
    {
        return buf_.cbegin();
    }

    const_iterator
    end() const noexcept
    {
        return buf_.end();
    }

    const_iterator
    cend() const noexcept
    {
        return buf_.cend();
    }
};



Seed
randomSeed();


Seed
generateSeed (std::string const& passPhrase);


template <>
boost::optional<Seed>
parseBase58 (std::string const& s);


boost::optional<Seed>
parseGenericSeed (std::string const& str);


std::string
seedAs1751 (Seed const& seed);


inline
std::string
toBase58 (Seed const& seed)
{
    return base58EncodeToken(TokenType::FamilySeed, seed.data(), seed.size());
}

}

#endif








