

#ifndef RIPPLE_PROTOCOL_STBASE_H_INCLUDED
#define RIPPLE_PROTOCOL_STBASE_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/Serializer.h>
#include <ostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <type_traits>
namespace ripple {

enum class JsonOptions {
    none         = 0,
    include_date = 1
};




class STBase
{
public:
    STBase();

    explicit
    STBase (SField const& n);

    virtual ~STBase() = default;

    STBase(const STBase& t) = default;
    STBase& operator= (const STBase& t);

    bool operator== (const STBase& t) const;
    bool operator!= (const STBase& t) const;

    virtual
    STBase*
    copy (std::size_t n, void* buf) const
    {
        return emplace(n, buf, *this);
    }

    virtual
    STBase*
    move (std::size_t n, void* buf)
    {
        return emplace(n, buf, std::move(*this));
    }

    template <class D>
    D&
    downcast()
    {
        D* ptr = dynamic_cast<D*> (this);
        if (ptr == nullptr)
            Throw<std::bad_cast> ();
        return *ptr;
    }

    template <class D>
    D const&
    downcast() const
    {
        D const * ptr = dynamic_cast<D const*> (this);
        if (ptr == nullptr)
            Throw<std::bad_cast> ();
        return *ptr;
    }

    virtual
    SerializedTypeID
    getSType() const;

    virtual
    std::string
    getFullText() const;

    virtual
    std::string
    getText() const;

    virtual
    Json::Value
    getJson (JsonOptions ) const;

    virtual
    void
    add (Serializer& s) const;

    virtual
    bool
    isEquivalent (STBase const& t) const;

    virtual
    bool
    isDefault() const;

    
    void
    setFName (SField const& n);

    SField const&
    getFName() const;

    void
    addFieldID (Serializer& s) const;

protected:
    SField const* fName;

    template <class T>
    static
    STBase*
    emplace(std::size_t n, void* buf, T&& val)
    {
        using U = std::decay_t<T>;
        if (sizeof(U) > n)
            return new U(std::forward<T>(val));
        return new(buf) U(std::forward<T>(val));
    }
};


std::ostream& operator<< (std::ostream& out, const STBase& t);

} 

#endif








