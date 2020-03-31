

#ifndef RIPPLE_TEST_CSF_DIGRAPH_H_INCLUDED
#define RIPPLE_TEST_CSF_DIGRAPH_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <fstream>
#include <unordered_map>
#include <type_traits>

namespace ripple {
namespace test {
namespace csf {

namespace detail {
struct NoEdgeData
{
};

}  


template <class Vertex, class EdgeData = detail::NoEdgeData>
class Digraph
{
    using Links = boost::container::flat_map<Vertex, EdgeData>;
    using Graph = boost::container::flat_map<Vertex, Links>;
    Graph graph_;

    Links empty;

public:
    
    bool
    connect(Vertex source, Vertex target, EdgeData e)
    {
        return graph_[source].emplace(target, e).second;
    }

    
    bool
    connect(Vertex source, Vertex target)
    {
        return connect(source, target, EdgeData{});
    }

    
    bool
    disconnect(Vertex source, Vertex target)
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
        {
            return it->second.erase(target) > 0;
        }
        return false;
    }

    
    boost::optional<EdgeData>
    edge(Vertex source, Vertex target) const
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
        {
            auto edgeIt = it->second.find(target);
            if (edgeIt != it->second.end())
                return edgeIt->second;
        }
        return boost::none;
    }

    
    bool
    connected(Vertex source, Vertex target) const
    {
        return edge(source, target) != boost::none;
    }

    
    auto
    outVertices() const
    {
        return boost::adaptors::transform(
            graph_,
            [](typename Graph::value_type const& v) { return v.first; });
    }

    
    auto
    outVertices(Vertex source) const
    {
        auto transform = [](typename Links::value_type const& link) {
            return link.first;
        };
        auto it = graph_.find(source);
        if (it != graph_.end())
            return boost::adaptors::transform(it->second, transform);

        return boost::adaptors::transform(empty, transform);
    }

    
    struct Edge
    {
        Vertex source;
        Vertex target;
        EdgeData data;
    };

    
    auto
    outEdges(Vertex source) const
    {
        auto transform = [source](typename Links::value_type const& link) {
            return Edge{source, link.first, link.second};
        };

        auto it = graph_.find(source);
        if (it != graph_.end())
            return boost::adaptors::transform(it->second, transform);

        return boost::adaptors::transform(empty, transform);
    }

    
    std::size_t
    outDegree(Vertex source) const
    {
        auto it = graph_.find(source);
        if (it != graph_.end())
            return it->second.size();
        return 0;
    }

    
    template <class VertexName>
    void
    saveDot(std::ostream & out, VertexName&& vertexName) const
    {
        out << "digraph {\n";
        for (auto const& vData : graph_)
        {
            auto const fromName = vertexName(vData.first);
            for (auto const& eData : vData.second)
            {
                auto const toName = vertexName(eData.first);
                out << fromName << " -> " << toName << ";\n";
            }
        }
        out << "}\n";
    }

    template <class VertexName>
    void
    saveDot(std::string const& fileName, VertexName&& vertexName) const
    {
        std::ofstream out(fileName);
        saveDot(out, std::forward<VertexName>(vertexName));
    }
};

}  
}  
}  
#endif








