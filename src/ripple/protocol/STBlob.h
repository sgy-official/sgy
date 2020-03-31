

#ifndef RIPPLE_PROTOCOL_STBLOB_H_INCLUDED
#define RIPPLE_PROTOCOL_STBLOB_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/STBase.h>
#include <cassert>
#include <cstring>
#include <memory>

namespace ripple {

class STBlob
    : public STBase
{
public:
    using value_type = Slice;

    STBlob () = default;
    STBlob (STBlob const& rhs)
        :STBase(rhs)
        , value_ (rhs.data (), rhs.size ())
    {
    }

    STBlob (SField const& f,
            void const* data, std::size_t size)
        : STBase(f), value_ (data, size)
    {
    }

    STBlob (SField const& f, Buffer&& b)
       : STBase(f), value_(std::move (b))
    {
    }

    STBlob (SField const& n)
        : STBase (n)
    {
    }

    STBlob (SerialIter&, SField const& name = sfGeneric);

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

    std::size_t
    size() const
    {
        return value_.size();
    }

    std::uint8_t const*
    data() const
    {
        return reinterpret_cast<
            std::uint8_t const*>(value_.data());
    }

    SerializedTypeID
    getSType () const override
    {
        return STI_VL;
    }

    std::string
    getText () const override;

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert ((fName->fieldType == STI_VL) ||
            (fName->fieldType == STI_ACCOUNT));
        s.addVL (value_.data (), value_.size ());
    }

    STBlob&
    operator= (Slice const& slice)
    {
        value_ = Buffer(slice.data(), slice.size());
        return *this;
    }

    value_type
    value() const noexcept
    {
        return value_;
    }

    STBlob&
    operator= (Buffer&& buffer)
    {
        value_ = std::move(buffer);
        return *this;
    }

    void
    setValue (Buffer&& b)
    {
        value_ = std::move (b);
    }

    bool
    isEquivalent (const STBase& t) const override;

    bool
    isDefault () const override
    {
        return value_.empty ();
    }

private:
    Buffer value_;
};

} 

#endif








