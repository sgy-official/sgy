

#ifndef RIPPLE_PROTOCOL_STACCOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_STACCOUNT_H_INCLUDED

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/STBase.h>
#include <string>

namespace ripple {

class STAccount final
    : public STBase
{
private:
    AccountID value_;
    bool default_;

public:
    using value_type = AccountID;

    STAccount ();
    STAccount (SField const& n);
    STAccount (SField const& n, Buffer&& v);
    STAccount (SerialIter& sit, SField const& name);
    STAccount (SField const& n, AccountID const& v);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace (n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace (n, buf, std::move(*this));
    }

    SerializedTypeID getSType () const override
    {
        return STI_ACCOUNT;
    }

    std::string getText () const override;

    void
    add (Serializer& s) const override
    {
        assert (fName->isBinary ());
        assert (fName->fieldType == STI_ACCOUNT);

        int const size = isDefault() ? 0 : uint160::bytes;
        s.addVL (value_.data(), size);
    }

    bool
    isEquivalent (const STBase& t) const override
    {
        auto const* const tPtr = dynamic_cast<STAccount const*>(&t);
        return tPtr && (default_ == tPtr->default_) && (value_ == tPtr->value_);
    }

    bool
    isDefault () const override
    {
        return default_;
    }

    STAccount&
    operator= (AccountID const& value)
    {
        setValue (value);
        return *this;
    }

    AccountID
    value() const noexcept
    {
        return value_;
    }

    void setValue (AccountID const& v)
    {
        value_ = v;
        default_ = false;
    }
};

} 

#endif








