

#ifndef RIPPLE_RESOURCE_CONSUMER_H_INCLUDED
#define RIPPLE_RESOURCE_CONSUMER_H_INCLUDED

#include <ripple/resource/Charge.h>
#include <ripple/resource/Disposition.h>

namespace ripple {
namespace Resource {

struct Entry;
class Logic;


class Consumer
{
private:
    friend class Logic;
    Consumer (Logic& logic, Entry& entry);

public:
    Consumer ();
    ~Consumer ();
    Consumer (Consumer const& other);
    Consumer& operator= (Consumer const& other);

    
    std::string to_string () const;

    
    bool isUnlimited () const;

    
    void elevate (std::string const& name);

    
    Disposition disposition() const;

    
    Disposition charge (Charge const& fee);

    
    bool warn();

    
    bool disconnect();

    
    int balance();

    Entry& entry();

private:
    Logic* m_logic;
    Entry* m_entry;
};

std::ostream& operator<< (std::ostream& os, Consumer const& v);

}
}

#endif








