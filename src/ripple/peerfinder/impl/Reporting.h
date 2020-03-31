

#ifndef RIPPLE_PEERFINDER_REPORTING_H_INCLUDED
#define RIPPLE_PEERFINDER_REPORTING_H_INCLUDED

namespace ripple {
namespace PeerFinder {


struct Reporting
{
    explicit Reporting() = default;

    static bool const params = true;

    static bool const crawl = true;

    static bool const nodes = true;

    static bool const dump_nodes = false;
};

}
}

#endif








