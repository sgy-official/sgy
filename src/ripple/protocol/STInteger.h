

#ifndef RIPPLE_PROTOCOL_STINTEGER_H_INCLUDED
#define RIPPLE_PROTOCOL_STINTEGER_H_INCLUDED

#include <ripple/protocol/STBase.h>

namespace ripple {

template <typename Integer>
class STInteger
    : public STBase
{
public:
    using value_type = Integer;

    explicit
    STInteger (Integer v)
        : value_ (v)
    { }

    STInteger (SField const& n, Integer v = 0)
        : STBase (n), value_ (v)
    { }

    STInteger(SerialIter& sit, SField const& name);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    SerializedTypeID
    getSType () const override;

    Json::Value
    getJson (JsonOptions) const override;

    std::string
    getText () const override;

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert (fName->fieldType == getSType ());
        s.addInteger (value_);
    }

    STInteger& operator= (value_type const& v)
    {
        value_ = v;
        return *this;
    }

    value_type value() const noexcept
    {
        return value_;
    }

    void
    setValue (Integer v)
    {
        value_ = v;
    }

    operator
    Integer () const
    {
        return value_;
    }

    virtual
    bool isDefault () const override
    {
        return value_ == 0;
    }

    bool
    isEquivalent (const STBase& t) const override
    {
        const STInteger* v = dynamic_cast<const STInteger*> (&t);
        return v && (value_ == v->value_);
    }

private:
    Integer value_;
};

using STUInt8  = STInteger<unsigned char>;
using STUInt16 = STInteger<std::uint16_t>;
using STUInt32 = STInteger<std::uint32_t>;
using STUInt64 = STInteger<std::uint64_t>;

} 

#endif








