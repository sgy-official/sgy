

#ifndef RIPPLE_JSON_JSONPROPERTYSTREAM_H_INCLUDED
#define RIPPLE_JSON_JSONPROPERTYSTREAM_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/beast/utility/PropertyStream.h>

namespace ripple {


class JsonPropertyStream : public beast::PropertyStream
{
public:
    Json::Value m_top;
    std::vector <Json::Value*> m_stack;

public:
    JsonPropertyStream ();
    Json::Value const& top() const;

protected:

    void map_begin () override;
    void map_begin (std::string const& key) override;
    void map_end () override;
    void add (std::string const& key, short value) override;
    void add (std::string const& key, unsigned short value) override;
    void add (std::string const& key, int value) override;
    void add (std::string const& key, unsigned int value) override;
    void add (std::string const& key, long value) override;
    void add (std::string const& key, float v) override;
    void add (std::string const& key, double v) override;
    void add (std::string const& key, std::string const& v) override;
    void array_begin () override;
    void array_begin (std::string const& key) override;
    void array_end () override;

    void add (short value) override;
    void add (unsigned short value) override;
    void add (int value) override;
    void add (unsigned int value) override;
    void add (long value) override;
    void add (float v) override;
    void add (double v) override;
    void add (std::string const& v) override;
};

}

#endif








