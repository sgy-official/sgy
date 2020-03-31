

#ifndef RIPPLE_PROTOCOL_STEXCHANGE_H_INCLUDED
#define RIPPLE_PROTOCOL_STEXCHANGE_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STBlob.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/STObject.h>
#include <ripple/basics/Blob.h>
#include <boost/optional.hpp>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ripple {


template <class U, class T>
struct STExchange;


template <class U, class T>
struct STExchange<STInteger<U>, T>
{
    explicit STExchange() = default;

    using value_type = U;

    static
    void
    get (boost::optional<T>& t,
        STInteger<U> const& u)
    {
        t = u.value();
    }

    static
    std::unique_ptr<STInteger<U>>
    set (SField const& f, T const& t)
    {
        return std::make_unique<
            STInteger<U>>(f, t);
    }
};

template <>
struct STExchange<STBlob, Slice>
{
    explicit STExchange() = default;

    using value_type = Slice;

    static
    void
    get (boost::optional<value_type>& t,
        STBlob const& u)
    {
        t.emplace (u.data(), u.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Slice const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }
};

template <>
struct STExchange<STBlob, Buffer>
{
    explicit STExchange() = default;

    using value_type = Buffer;

    static
    void
    get (boost::optional<Buffer>& t,
        STBlob const& u)
    {
        t.emplace (
            u.data(), u.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Buffer const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }

    static
    std::unique_ptr<STBlob>
    set (TypedField<STBlob> const& f,
        Buffer&& t)
    {
        return std::make_unique<STBlob>(
            f, std::move(t));
    }
};




template <class T, class U>
boost::optional<T>
get (STObject const& st,
    TypedField<U> const& f)
{
    boost::optional<T> t;
    STBase const* const b =
        st.peekAtPField(f);
    if (! b)
        return t;
    auto const id = b->getSType();
    if (id == STI_NOTPRESENT)
        return t;
    auto const u =
        dynamic_cast<U const*>(b);
    if (! u)
        Throw<std::runtime_error> (
            "Wrong field type");
    STExchange<U, T>::get(t, *u);
    return t;
}

template <class U>
boost::optional<typename STExchange<
    U, typename U::value_type>::value_type>
get (STObject const& st,
    TypedField<U> const& f)
{
    return get<typename U::value_type>(st, f);
}



template <class U, class T>
void
set (STObject& st,
    TypedField<U> const& f, T&& t)
{
    st.set(STExchange<U,
        typename std::decay<T>::type>::set(
            f, std::forward<T>(t)));
}


template <class Init>
void
set (STObject& st,
    TypedField<STBlob> const& f,
        std::size_t size, Init&& init)
{
    st.set(std::make_unique<STBlob>(
        f, size, init));
}


template <class = void>
void
set (STObject& st,
    TypedField<STBlob> const& f,
        void const* data, std::size_t size)
{
    st.set(std::make_unique<STBlob>(
        f, data, size));
}


template <class U>
void
erase (STObject& st,
    TypedField<U> const& f)
{
    st.makeFieldAbsent(f);
}

} 

#endif








