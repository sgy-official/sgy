

#ifndef RIPPLE_PEERFINDER_SIM_MESSAGE_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_MESSAGE_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {

class Message
{
public:
    explicit Message (Endpoints const& endpoints)
        : m_payload (endpoints)
        { }
    Endpoints const& payload () const
        { return m_payload; }
private:
    Endpoints m_payload;
};

}
}
}

#endif








