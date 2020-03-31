

#ifndef RIPPLE_RESOURCE_IMPORT_H_INCLUDED
#define RIPPLE_RESOURCE_IMPORT_H_INCLUDED

#include <ripple/resource/impl/Entry.h>
#include <ripple/resource/Consumer.h>

namespace ripple {
namespace Resource {


struct Import
{
    struct Item
    {
        explicit Item() = default;

        int balance;
        Consumer consumer;
    };

    Import (int = 0)
        : whenExpires ()
    {
    }

    clock_type::time_point whenExpires;

    std::vector <Item> items;
};

}
}

#endif








