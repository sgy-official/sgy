

#ifndef RIPPLE_JSON_TO_STRING_H_INCLUDED
#define RIPPLE_JSON_TO_STRING_H_INCLUDED

#include <string>
#include <ostream>

namespace Json {

class Value;


std::string to_string (Value const&);


std::string pretty (Value const&);


std::ostream& operator<< (std::ostream&, const Value& root);

} 

#endif 








