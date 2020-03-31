

#ifndef RIPPLE_PROTOCOL_TOKENS_H_INCLUDED
#define RIPPLE_PROTOCOL_TOKENS_H_INCLUDED

#include <boost/optional.hpp>
#include <cstdint>
#include <string>

namespace ripple {

enum class TokenType : std::uint8_t
{
    None             = 1,       
    NodePublic       = 28,
    NodePrivate      = 32,
    AccountID        = 0,
    AccountPublic    = 35,
    AccountSecret    = 34,
    FamilyGenerator  = 41,      
    FamilySeed       = 33
};

template <class T>
boost::optional<T>
parseBase58 (std::string const& s);

template<class T>
boost::optional<T>
parseBase58 (TokenType type, std::string const& s);

template <class T>
boost::optional<T>
parseHex (std::string const& s);

template <class T>
boost::optional<T>
parseHexOrBase58 (std::string const& s);



std::string
base58EncodeToken (TokenType type, void const* token, std::size_t size);


std::string
base58EncodeTokenBitcoin (TokenType type, void const* token, std::size_t size);


std::string
decodeBase58Token(std::string const& s, TokenType type);


std::string
decodeBase58TokenBitcoin(std::string const& s, TokenType type);

} 

#endif








