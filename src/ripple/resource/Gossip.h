

#ifndef RIPPLE_RESOURCE_GOSSIP_H_INCLUDED
#define RIPPLE_RESOURCE_GOSSIP_H_INCLUDED

#include <ripple/beast/net/IPEndpoint.h>

namespace ripple {
namespace Resource {


struct Gossip
{
    explicit Gossip() = default;

    
    struct Item
    {
        explicit Item() = default;

        int balance;
        beast::IP::Endpoint address;
    };

    std::vector <Item> items;
};

}
}

#endif








