

#ifndef RIPPLE_RESOURCE_CHARGE_H_INCLUDED
#define RIPPLE_RESOURCE_CHARGE_H_INCLUDED

#include <ios>
#include <string>

namespace ripple {
namespace Resource {


class Charge
{
public:
    
    using value_type = int;

    Charge () = delete;

    
    Charge (value_type cost, std::string const& label = std::string());

    
    std::string const& label() const;

    
    value_type cost () const;

    
    std::string to_string () const;

    bool operator== (Charge const&) const;
    bool operator!= (Charge const&) const;

private:
    value_type m_cost;
    std::string m_label;
};

std::ostream& operator<< (std::ostream& os, Charge const& v);

}
}

#endif








