

#ifndef RIPPLE_PEERFINDER_SOURCESTRINGS_H_INCLUDED
#define RIPPLE_PEERFINDER_SOURCESTRINGS_H_INCLUDED

#include <ripple/peerfinder/impl/Source.h>
#include <memory>

namespace ripple {
namespace PeerFinder {


class SourceStrings : public Source
{
public:
    explicit SourceStrings() = default;

    using Strings = std::vector <std::string>;

    static std::shared_ptr<Source>
    New (std::string const& name, Strings const& strings);
};

}
}

#endif








