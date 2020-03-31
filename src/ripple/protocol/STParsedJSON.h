

#ifndef RIPPLE_PROTOCOL_STPARSEDJSON_H_INCLUDED
#define RIPPLE_PROTOCOL_STPARSEDJSON_H_INCLUDED

#include <ripple/protocol/STArray.h>
#include <boost/optional.hpp>

namespace ripple {


class STParsedJSONObject
{
public:
    
    STParsedJSONObject (std::string const& name, Json::Value const& json);

    STParsedJSONObject () = delete;
    STParsedJSONObject (STParsedJSONObject const&) = delete;
    STParsedJSONObject& operator= (STParsedJSONObject const&) = delete;
    ~STParsedJSONObject () = default;

    
    boost::optional <STObject> object;

    
    Json::Value error;
};


class STParsedJSONArray
{
public:
    
    STParsedJSONArray (std::string const& name, Json::Value const& json);

    STParsedJSONArray () = delete;
    STParsedJSONArray (STParsedJSONArray const&) = delete;
    STParsedJSONArray& operator= (STParsedJSONArray const&) = delete;
    ~STParsedJSONArray () = default;

    
    boost::optional <STArray> array;

    
    Json::Value error;
};



} 

#endif








