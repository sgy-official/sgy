

#ifndef RIPPLE_PEERFINDER_SIM_GRAPHALGORITHMS_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_GRAPHALGORITHMS_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {

template <typename Vertex>
struct VertexTraits;



template <typename Vertex, typename Function>
void breadth_first_traverse (Vertex& start, Function f)
{
    using Traits = VertexTraits <Vertex>;
    using Edges  = typename Traits::Edges;
    using Edge   = typename Traits::Edge;

    using Probe = std::pair <Vertex*, int>;
    using Work = std::deque <Probe>;
    using Visited = std::set <Vertex*>;
    Work work;
    Visited visited;
    work.emplace_back (&start, 0);
    int diameter (0);
    while (! work.empty ())
    {
        Probe const p (work.front());
        work.pop_front ();
        if (visited.find (p.first) != visited.end ())
            continue;
        diameter = std::max (p.second, diameter);
        visited.insert (p.first);
        for (typename Edges::iterator iter (
            Traits::edges (*p.first).begin());
                iter != Traits::edges (*p.first).end(); ++iter)
        {
            Vertex* v (Traits::vertex (*iter));
            if (visited.find (v) != visited.end())
                continue;
            if (! iter->closed())
                work.emplace_back (v, p.second + 1);
        }
        f (*p.first, diameter);
    }
}


}
}
}

#endif








