
#ifndef BEAST_DOC_DEBUG_HPP
#define BEAST_DOC_DEBUG_HPP

namespace beast {

#if BEAST_DOXYGEN

using doc_type = int;

enum doc_enum
{
    one,

    two
};

enum class doc_enum_class : unsigned
{
    one,

    two
};

void doc_func();

struct doc_class
{
    void func();
};

namespace nested {

using nested_doc_type = int;

enum nested_doc_enum
{
    one,

    two
};

enum class nested_doc_enum_class : unsigned
{
    one,

    two
};

void nested_doc_func();

struct nested_doc_class
{
    void func();
};

} 


void doc_debug();

namespace nested {


void nested_doc_debug();

} 

#endif

} 

#endif






