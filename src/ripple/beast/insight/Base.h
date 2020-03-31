

#ifndef BEAST_INSIGHT_BASE_H_INCLUDED
#define BEAST_INSIGHT_BASE_H_INCLUDED

#include <memory>

namespace beast {
namespace insight {


class Base
{
public:
    virtual ~Base () = 0;
    Base() = default;
    Base(Base const&) = default;
    Base& operator=(Base const&) = default;
};

}
}

#endif








