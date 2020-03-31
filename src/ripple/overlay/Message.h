

#ifndef RIPPLE_OVERLAY_MESSAGE_H_INCLUDED
#define RIPPLE_OVERLAY_MESSAGE_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>

namespace ripple {



class Message : public std::enable_shared_from_this <Message>
{
public:
    using pointer = std::shared_ptr<Message>;

public:
    
    static std::size_t constexpr kHeaderBytes = 6;

    
    static std::size_t constexpr kMaxMessageSize = 64 * 1024 * 1024;

    Message (::google::protobuf::Message const& message, int type);

    
    std::vector <uint8_t> const&
    getBuffer () const
    {
        return mBuffer;
    }

    
    std::size_t
    getCategory () const
    {
        return mCategory;
    }

    
    bool operator == (Message const& other) const;

    
    
    static unsigned getLength (std::vector <uint8_t> const& buf);

    template <class FwdIter>
    static
    std::enable_if_t<std::is_same<typename
        FwdIter::value_type, std::uint8_t>::value, std::size_t>
    size (FwdIter first, FwdIter last)
    {
        if (std::distance(first, last) <
                Message::kHeaderBytes)
            return 0;
        std::size_t n;
        n  = std::size_t{*first++} << 24;
        n += std::size_t{*first++} << 16;
        n += std::size_t{*first++} <<  8;
        n += std::size_t{*first};
        return n;
    }

    template <class BufferSequence>
    static
    std::size_t
    size (BufferSequence const& buffers)
    {
        return size(buffers_begin(buffers),
            buffers_end(buffers));
    }
    

    
    
    static int getType (std::vector <uint8_t> const& buf);

    template <class FwdIter>
    static
    std::enable_if_t<std::is_same<typename
        FwdIter::value_type, std::uint8_t>::value, int>
    type (FwdIter first, FwdIter last)
    {
        if (std::distance(first, last) <
                Message::kHeaderBytes)
            return 0;
        return (int{*std::next(first, 4)} << 8) |
            *std::next(first, 5);
    }

    template <class BufferSequence>
    static
    int
    type (BufferSequence const& buffers)
    {
        return type(buffers_begin(buffers),
            buffers_end(buffers));
    }
    

private:
    template <class BufferSequence, class Value = std::uint8_t>
    static
    boost::asio::buffers_iterator<BufferSequence, Value>
    buffers_begin (BufferSequence const& buffers)
    {
        return boost::asio::buffers_iterator<
            BufferSequence, Value>::begin (buffers);
    }

    template <class BufferSequence, class Value = std::uint8_t>
    static
    boost::asio::buffers_iterator<BufferSequence, Value>
    buffers_end (BufferSequence const& buffers)
    {
        return boost::asio::buffers_iterator<
            BufferSequence, Value>::end (buffers);
    }

    void encodeHeader (unsigned size, int type);

    std::vector <uint8_t> mBuffer;

    std::size_t mCategory;
};

}

#endif








