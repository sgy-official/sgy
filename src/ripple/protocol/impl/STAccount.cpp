

#include <ripple/protocol/STAccount.h>

namespace ripple {

STAccount::STAccount ()
    : STBase ()
    , value_ (beast::zero)
    , default_ (true)
{
}

STAccount::STAccount (SField const& n)
    : STBase (n)
    , value_ (beast::zero)
    , default_ (true)
{
}

STAccount::STAccount (SField const& n, Buffer&& v)
    : STAccount (n)
{
    if (v.empty())
        return;  

    if (v.size() != uint160::bytes)
        Throw<std::runtime_error> ("Invalid STAccount size");

    default_ = false;
    memcpy (value_.begin(), v.data (), uint160::bytes);
}

STAccount::STAccount (SerialIter& sit, SField const& name)
    : STAccount(name, sit.getVLBuffer())
{
}

STAccount::STAccount (SField const& n, AccountID const& v)
    : STBase (n)
    , value_(v)
    , default_ (false)
{
}

std::string STAccount::getText () const
{
    if (isDefault())
        return "";
    return toBase58 (value());
}

} 
























