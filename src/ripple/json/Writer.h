

#ifndef RIPPLE_JSON_WRITER_H_INCLUDED
#define RIPPLE_JSON_WRITER_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/basics/ToString.h>
#include <ripple/json/Output.h>
#include <ripple/json/json_value.h>
#include <memory>

namespace Json {



class Writer
{
public:
    enum CollectionType {array, object};

    explicit Writer (Output const& output);
    Writer(Writer&&) noexcept;
    Writer& operator=(Writer&&) noexcept;

    ~Writer();

    
    void startRoot (CollectionType);

    
    void startAppend (CollectionType);

    
    void startSet (CollectionType, std::string const& key);

    
    void finish ();

    
    void finishAll ();

    
    template <typename Scalar>
    void append (Scalar t)
    {
        rawAppend();
        output (t);
    }

    
    void rawAppend();

    
    template <typename Type>
    void set (std::string const& tag, Type t)
    {
        rawSet (tag);
        output (t);
    }

    
    void rawSet (std::string const& key);


    
    void output (std::string const&);

    
    void output (char const*);

    
    void output (Json::Value const&);

    
    void output (std::nullptr_t);

    
    void output (float);

    
    void output (double);

    
    void output (bool);

    
    template <typename Type>
    void output (Type t)
    {
        implOutput (std::to_string (t));
    }

    void output (Json::StaticString const& t)
    {
        output (t.c_str());
    }

private:
    class Impl;
    std::unique_ptr <Impl> impl_;

    void implOutput (std::string const&);
};

inline void check (bool condition, std::string const& message)
{
    if (! condition)
        ripple::Throw<std::logic_error> (message);
}

} 

#endif








