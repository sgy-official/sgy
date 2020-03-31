

#ifndef RIPPLE_PROTOCOL_INNER_OBJECT_FORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_INNER_OBJECT_FORMATS_H_INCLUDED

#include <ripple/protocol/KnownFormats.h>

namespace ripple {


class InnerObjectFormats : public KnownFormats <int>
{
private:
    
    InnerObjectFormats ();

public:
    static InnerObjectFormats const& getInstance ();

    SOTemplate const* findSOTemplateBySField (SField const& sField) const;
};

} 

#endif








