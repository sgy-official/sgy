

#ifndef RIPPLE_PROTOCOL_SERIALIZER_H_INCLUDED
#define RIPPLE_PROTOCOL_SERIALIZER_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Blob.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Buffer.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/Slice.h>
#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/protocol/SField.h>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace ripple {

class CKey; 

class Serializer
{
private:
    Blob mData;

public:
    explicit
    Serializer (int n = 256)
    {
        mData.reserve (n);
    }

    Serializer (void const* data, std::size_t size)
    {
        mData.resize(size);

        if (size)
        {
            assert(data != nullptr);
            std::memcpy(mData.data(), data, size);
        }
    }

    Slice slice() const noexcept
    {
        return Slice(mData.data(), mData.size());
    }

    std::size_t
    size() const noexcept
    {
        return mData.size();
    }

    void const*
    data() const noexcept
    {
        return mData.data();
    }

    int add8 (unsigned char byte);
    int add16 (std::uint16_t);
    int add32 (std::uint32_t);      
    int add64 (std::uint64_t);      
    int add128 (const uint128&);    
    int add256 (uint256 const& );       

    template <typename Integer>
    int addInteger(Integer);

    template <int Bits, class Tag>
    int addBitString(base_uint<Bits, Tag> const& v) {
        int ret = mData.size ();
        mData.insert (mData.end (), v.begin (), v.end ());
        return ret;
    }

    template <class Tag>
    int add160 (base_uint<160, Tag> const& i)
    {
        return addBitString<160, Tag>(i);
    }

    int addRaw (Blob const& vector);
    int addRaw (const void* ptr, int len);
    int addRaw (const Serializer& s);
    int addZeros (size_t uBytes);

    int addVL (Blob const& vector);
    int addVL (Slice const& slice);
    template<class Iter>
    int addVL (Iter begin, Iter end, int len);
    int addVL (const void* ptr, int len);

    bool get8 (int&, int offset) const;
    bool get256 (uint256&, int offset) const;

    template <typename Integer>
    bool getInteger(Integer& number, int offset) {
        static const auto bytes = sizeof(Integer);
        if ((offset + bytes) > mData.size ())
            return false;
        number = 0;

        auto ptr = &mData[offset];
        for (auto i = 0; i < bytes; ++i)
        {
            if (i)
                number <<= 8;
            number |= *ptr++;
        }
        return true;
    }

    template <int Bits, typename Tag = void>
    bool getBitString(base_uint<Bits, Tag>& data, int offset) const {
        auto success = (offset + (Bits / 8)) <= mData.size ();
        if (success)
            memcpy (data.begin (), & (mData.front ()) + offset, (Bits / 8));
        return success;
    }

    template <class Tag>
    bool get160 (base_uint<160, Tag>& o, int offset) const
    {
        return getBitString<160, Tag>(o, offset);
    }

    bool getRaw (Blob&, int offset, int length) const;
    Blob getRaw (int offset, int length) const;

    bool getVL (Blob& objectVL, int offset, int& length) const;
    bool getVLLength (int& length, int offset) const;

    int addFieldID (int type, int name);
    int addFieldID (SerializedTypeID type, int name)
    {
        return addFieldID (safe_cast<int> (type), name);
    }

    uint256 getSHA512Half() const;

    Blob const& peekData () const
    {
        return mData;
    }
    Blob getData () const
    {
        return mData;
    }
    Blob& modData ()
    {
        return mData;
    }

    int getDataLength () const
    {
        return mData.size ();
    }
    const void* getDataPtr () const
    {
        return mData.data();
    }
    void* getDataPtr ()
    {
        return mData.data();
    }
    int getLength () const
    {
        return mData.size ();
    }
    std::string getString () const
    {
        return std::string (static_cast<const char*> (getDataPtr ()), size ());
    }
    void secureErase ()
    {
        beast::secure_erase(mData.data(), mData.size());
        mData.clear ();
    }
    void erase ()
    {
        mData.clear ();
    }
    bool chop (int num);

    Blob ::iterator begin ()
    {
        return mData.begin ();
    }
    Blob ::iterator end ()
    {
        return mData.end ();
    }
    Blob ::const_iterator begin () const
    {
        return mData.begin ();
    }
    Blob ::const_iterator end () const
    {
        return mData.end ();
    }
    void reserve (size_t n)
    {
        mData.reserve (n);
    }
    void resize (size_t n)
    {
        mData.resize (n);
    }
    size_t capacity () const
    {
        return mData.capacity ();
    }

    bool operator== (Blob const& v)
    {
        return v == mData;
    }
    bool operator!= (Blob const& v)
    {
        return v != mData;
    }
    bool operator== (const Serializer& v)
    {
        return v.mData == mData;
    }
    bool operator!= (const Serializer& v)
    {
        return v.mData != mData;
    }

    std::string getHex () const
    {
        std::stringstream h;

        for (unsigned char const& element : mData)
        {
            h <<
                std::setw (2) <<
                std::hex <<
                std::setfill ('0') <<
                safe_cast<unsigned int>(element);
        }
        return h.str ();
    }

    static int decodeLengthLength (int b1);
    static int decodeVLLength (int b1);
    static int decodeVLLength (int b1, int b2);
    static int decodeVLLength (int b1, int b2, int b3);
private:
    static int lengthVL (int length)
    {
        return length + encodeLengthLength (length);
    }
    static int encodeLengthLength (int length); 
    int addEncoded (int length);
};

template<class Iter>
int Serializer::addVL(Iter begin, Iter end, int len)
{
    int ret = addEncoded(len);
    for (; begin != end; ++begin)
    {
        addRaw(begin->data(), begin->size());
#ifndef NDEBUG
        len -= begin->size();
#endif
    }
    assert(len == 0);
    return ret;
}


class SerialIter
{
private:
    std::uint8_t const* p_;
    std::size_t remain_;
    std::size_t used_ = 0;

public:
    SerialIter (void const* data,
            std::size_t size) noexcept;

    SerialIter (Slice const& slice)
        : SerialIter(slice.data(), slice.size())
    {
    }

    template<int N>
    explicit SerialIter (std::uint8_t const (&data)[N])
        : SerialIter(&data[0], N)
    {
        static_assert (N > 0, "");
    }

    std::size_t
    empty() const noexcept
    {
        return remain_ == 0;
    }

    void
    reset() noexcept;

    int
    getBytesLeft() const noexcept
    {
        return static_cast<int>(remain_);
    }

    unsigned char
    get8();

    std::uint16_t
    get16();

    std::uint32_t
    get32();

    std::uint64_t
    get64();

    template <int Bits, class Tag = void>
    base_uint<Bits, Tag>
    getBitString();

    uint128
    get128()
    {
        return getBitString<128>();
    }

    uint160
    get160()
    {
        return getBitString<160>();
    }

    uint256
    get256()
    {
        return getBitString<256>();
    }

    void
    getFieldID (int& type, int& name);

    int
    getVLDataLength ();

    Slice
    getSlice (std::size_t bytes);

    Blob
    getRaw (int size);

    Blob
    getVL();

    void
    skip (int num);

    Buffer
    getVLBuffer();

    template<class T>
    T getRawHelper (int size);
};

template <int Bits, class Tag>
base_uint<Bits, Tag>
SerialIter::getBitString()
{
    base_uint<Bits, Tag> u;
    auto const n = Bits/8;
    if (remain_ < n)
        Throw<std::runtime_error> (
            "invalid SerialIter getBitString");
    std::memcpy (u.begin(), p_, n);
    p_ += n;
    used_ += n;
    remain_ -= n;
    return u;
}

} 

#endif








