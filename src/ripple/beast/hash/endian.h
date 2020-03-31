

#ifndef BEAST_HASH_ENDIAN_H_INCLUDED
#define BEAST_HASH_ENDIAN_H_INCLUDED

namespace beast {

enum class endian
{
#ifdef _MSC_VER
    big    = 1,
    little = 0,
    native = little
#else
    native = __BYTE_ORDER__,
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__
#endif
};

#ifndef __INTELLISENSE__
static_assert(endian::native == endian::little ||
              endian::native == endian::big,
              "endian::native shall be one of endian::little or endian::big");

static_assert(endian::big != endian::little,
              "endian::big and endian::little shall have different values");
#endif

} 

#endif








