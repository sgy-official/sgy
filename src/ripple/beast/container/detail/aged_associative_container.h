

#ifndef BEAST_CONTAINER_DETAIL_AGED_ASSOCIATIVE_CONTAINER_H_INCLUDED
#define BEAST_CONTAINER_DETAIL_AGED_ASSOCIATIVE_CONTAINER_H_INCLUDED

#include <type_traits>

namespace beast {
namespace detail {

template <bool maybe_map>
struct aged_associative_container_extract_t
{
    explicit aged_associative_container_extract_t() = default;

    template <class Value>
    decltype (Value::first) const&
    operator() (Value const& value) const
    {
        return value.first;
    }
};

template <>
struct aged_associative_container_extract_t <false>
{
    explicit aged_associative_container_extract_t() = default;

    template <class Value>
    Value const&
    operator() (Value const& value) const
    {
        return value;
    }
};

}
}

#endif








