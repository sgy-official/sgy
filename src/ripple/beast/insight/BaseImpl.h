

#ifndef BEAST_INSIGHT_BASEIMPL_H_INCLUDED
#define BEAST_INSIGHT_BASEIMPL_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace beast {
namespace insight {


class BaseImpl
{
public:
    using ptr = std::shared_ptr <BaseImpl>;

    virtual ~BaseImpl () = 0;
};

}
}

#endif








