

#ifndef RIPPLE_JSON_JSON_WRITER_H_INCLUDED
#define RIPPLE_JSON_JSON_WRITER_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <ripple/json/json_value.h>
#include <ostream>
#include <vector>

namespace Json
{

class Value;


class WriterBase
{
public:
    virtual ~WriterBase () {}
    virtual std::string write ( const Value& root ) = 0;
};



class FastWriter : public WriterBase
{
public:
    FastWriter () = default;
    virtual ~FastWriter () {}

public: 
    std::string write ( const Value& root ) override;

private:
    void writeValue ( const Value& value );

    std::string document_;
};


class StyledWriter: public WriterBase
{
public:
    StyledWriter ();
    virtual ~StyledWriter () {}

public: 
    
    std::string write ( const Value& root ) override;

private:
    void writeValue ( const Value& value );
    void writeArrayValue ( const Value& value );
    bool isMultineArray ( const Value& value );
    void pushValue ( std::string const& value );
    void writeIndent ();
    void writeWithIndent ( std::string const& value );
    void indent ();
    void unindent ();

    using ChildValues = std::vector<std::string>;

    ChildValues childValues_;
    std::string document_;
    std::string indentString_;
    int rightMargin_;
    int indentSize_;
    bool addChildValues_;
};


class StyledStreamWriter
{
public:
    StyledStreamWriter ( std::string indentation = "\t" );
    ~StyledStreamWriter () {}

public:
    
    void write ( std::ostream& out, const Value& root );

private:
    void writeValue ( const Value& value );
    void writeArrayValue ( const Value& value );
    bool isMultineArray ( const Value& value );
    void pushValue ( std::string const& value );
    void writeIndent ();
    void writeWithIndent ( std::string const& value );
    void indent ();
    void unindent ();

    using ChildValues = std::vector<std::string>;

    ChildValues childValues_;
    std::ostream* document_;
    std::string indentString_;
    int rightMargin_;
    std::string indentation_;
    bool addChildValues_;
};

std::string valueToString ( Int value );
std::string valueToString ( UInt value );
std::string valueToString ( double value );
std::string valueToString ( bool value );
std::string valueToQuotedString ( const char* value );

std::ostream& operator<< ( std::ostream&, const Value& root );


namespace detail {

template <class Write>
void
write_string(Write const& write, std::string const& s)
{
    write(s.data(), s.size());
}

template <class Write>
void
write_value(Write const& write, Value const& value)
{
    switch (value.type())
    {
        case nullValue:
            write("null", 4);
            break;

        case intValue:
            write_string(write, valueToString(value.asInt()));
            break;

        case uintValue:
            write_string(write, valueToString(value.asUInt()));
            break;

        case realValue:
            write_string(write, valueToString(value.asDouble()));
            break;

        case stringValue:
            write_string(write, valueToQuotedString(value.asCString()));
            break;

        case booleanValue:
            write_string(write, valueToString(value.asBool()));
            break;

        case arrayValue:
        {
            write("[", 1);
            int const size = value.size();
            for (int index = 0; index < size; ++index)
            {
                if (index > 0)
                    write(",", 1);
                write_value(write, value[index]);
            }
            write("]", 1);
            break;
        }

        case objectValue:
        {
            Value::Members const members = value.getMemberNames();
            write("{", 1);
            for (auto it = members.begin(); it != members.end(); ++it)
            {
                std::string const& name = *it;
                if (it != members.begin())
                    write(",", 1);

                write_string(write, valueToQuotedString(name.c_str()));
                write(":", 1);
                write_value(write, value[name]);
            }
            write("}", 1);
            break;
        }
    }
}

}  


template <class Write>
void
stream(Json::Value const& jv, Write const& write)
{
    detail::write_value(write, jv);
    write("\n", 1);
}


class Compact
{
    Json::Value jv_;

public:
    
    Compact(Json::Value&& jv) : jv_{std::move(jv)}
    {
    }

    friend std::ostream&
    operator<<(std::ostream& o, Compact const& cJv)
    {
        detail::write_value(
            [&o](void const* data, std::size_t n) {
                o.write(static_cast<char const*>(data), n);
            },
            cJv.jv_);
        return o;
    }
};

}  

#endif 








