

#include <ripple/basics/safe_cast.h>
#include <ripple/overlay/Message.h>
#include <ripple/overlay/impl/TrafficCount.h>
#include <cstdint>

namespace ripple {

Message::Message (::google::protobuf::Message const& message, int type)
{
    unsigned const messageBytes = message.ByteSize ();

    assert (messageBytes != 0);

    mBuffer.resize (kHeaderBytes + messageBytes);

    encodeHeader (messageBytes, type);

    if (messageBytes != 0)
    {
        message.SerializeToArray (&mBuffer [Message::kHeaderBytes], messageBytes);
    }

    mCategory = TrafficCount::categorize(message, type, false);
}

bool Message::operator== (Message const& other) const
{
    return mBuffer == other.mBuffer;
}

unsigned Message::getLength (std::vector <uint8_t> const& buf)
{
    unsigned result;

    if (buf.size () >= Message::kHeaderBytes)
    {
        result = buf [0];
        result <<= 8;
        result |= buf [1];
        result <<= 8;
        result |= buf [2];
        result <<= 8;
        result |= buf [3];
    }
    else
    {
        result = 0;
    }

    return result;
}

int Message::getType (std::vector<uint8_t> const& buf)
{
    if (buf.size () < Message::kHeaderBytes)
        return 0;

    int ret = buf[4];
    ret <<= 8;
    ret |= buf[5];
    return ret;
}

void Message::encodeHeader (unsigned size, int type)
{
    assert (mBuffer.size () >= Message::kHeaderBytes);
    mBuffer[0] = static_cast<std::uint8_t> ((size >> 24) & 0xFF);
    mBuffer[1] = static_cast<std::uint8_t> ((size >> 16) & 0xFF);
    mBuffer[2] = static_cast<std::uint8_t> ((size >> 8) & 0xFF);
    mBuffer[3] = static_cast<std::uint8_t> (size & 0xFF);
    mBuffer[4] = static_cast<std::uint8_t> ((type >> 8) & 0xFF);
    mBuffer[5] = static_cast<std::uint8_t> (type & 0xFF);
}

}
























