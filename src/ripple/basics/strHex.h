


#ifndef RIPPLE_BASICS_STRHEX_H_INCLUDED
#define RIPPLE_BASICS_STRHEX_H_INCLUDED

#include <cassert>
#include <string>

#include <boost/algorithm/hex.hpp>
#include <boost/endian/conversion.hpp>

namespace ripple {


inline
char
charHex (unsigned int digit)
{
    static
    char const xtab[] = "0123456789ABCDEF";

    assert (digit < 16);

    return xtab[digit];
}



int
charUnHex (unsigned char c);

inline
int
charUnHex (char c)
{
    return charUnHex (static_cast<unsigned char>(c));
}


template <class FwdIt>
std::string
strHex(FwdIt begin, FwdIt end)
{
    static_assert(
        std::is_convertible<
            typename std::iterator_traits<FwdIt>::iterator_category,
            std::forward_iterator_tag>::value,
        "FwdIt must be a forward iterator");
    std::string result;
    result.reserve(2 * std::distance(begin, end));
    boost::algorithm::hex(begin, end, std::back_inserter(result));
    return result;
}

template <class T, class = decltype(std::declval<T>().begin())>
std::string strHex(T const& from)
{
    return strHex(from.begin(), from.end());
}

inline std::string strHex (const std::uint64_t uiHost)
{
    uint64_t uBig    = boost::endian::native_to_big (uiHost);

    auto const begin = (unsigned char*) &uBig;
    auto const end   = begin + sizeof(uBig);
    return strHex(begin, end);
}
}

#endif








