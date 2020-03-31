

#ifndef RIPPLE_SERVER_WRITER_H_INCLUDED
#define RIPPLE_SERVER_WRITER_H_INCLUDED

#include <boost/asio/buffer.hpp>
#include <functional>
#include <vector>

namespace ripple {

class Writer
{
public:
    virtual ~Writer() = default;

    
    virtual
    bool
    complete() = 0;

    
    virtual
    void
    consume (std::size_t bytes) = 0;

    
    virtual
    bool
    prepare (std::size_t bytes,
        std::function<void(void)> resume) = 0;

    
    virtual
    std::vector<boost::asio::const_buffer>
    data() = 0;
};

} 

#endif








