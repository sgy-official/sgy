

#ifndef BEAST_UTILITY_PROPERTYSTREAM_H_INCLUDED
#define BEAST_UTILITY_PROPERTYSTREAM_H_INCLUDED

#include <ripple/beast/core/List.h>

#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace beast {



class PropertyStream
{
public:
    class Map;
    class Set;
    class Source;

    PropertyStream () = default;
    virtual ~PropertyStream () = default;

protected:
    virtual void map_begin () = 0;
    virtual void map_begin (std::string const& key) = 0;
    virtual void map_end () = 0;

    virtual void add (std::string const& key, std::string const& value) = 0;

    void add (std::string const& key, char const* value)
    {
        add (key, std::string (value));
    }

    template <typename Value>
    void lexical_add (std::string const &key, Value value)
    {
        std::stringstream ss;
        ss << value;
        add (key, ss.str());
    }

    virtual void add (std::string const& key, bool value);
    virtual void add (std::string const& key, char value);
    virtual void add (std::string const& key, signed char value);
    virtual void add (std::string const& key, unsigned char value);
    virtual void add (std::string const& key, wchar_t value);
#if 0
    virtual void add (std::string const& key, char16_t value);
    virtual void add (std::string const& key, char32_t value);
#endif
    virtual void add (std::string const& key, short value);
    virtual void add (std::string const& key, unsigned short value);
    virtual void add (std::string const& key, int value);
    virtual void add (std::string const& key, unsigned int value);
    virtual void add (std::string const& key, long value);
    virtual void add (std::string const& key, unsigned long value);
    virtual void add (std::string const& key, long long value);
    virtual void add (std::string const& key, unsigned long long value);
    virtual void add (std::string const& key, float value);
    virtual void add (std::string const& key, double value);
    virtual void add (std::string const& key, long double value);

    virtual void array_begin () = 0;
    virtual void array_begin (std::string const& key) = 0;
    virtual void array_end () = 0;

    virtual void add (std::string const& value) = 0;

    void add (char const* value)
    {
        add (std::string (value));
    }

    template <typename Value>
    void lexical_add (Value value)
    {
        std::stringstream ss;
        ss << value;
        add (ss.str());
    }

    virtual void add (bool value);
    virtual void add (char value);
    virtual void add (signed char value);
    virtual void add (unsigned char value);
    virtual void add (wchar_t value);
#if 0
    virtual void add (char16_t value);
    virtual void add (char32_t value);
#endif
    virtual void add (short value);
    virtual void add (unsigned short value);
    virtual void add (int value);
    virtual void add (unsigned int value);
    virtual void add (long value);
    virtual void add (unsigned long value);
    virtual void add (long long value);
    virtual void add (unsigned long long value);
    virtual void add (float value);
    virtual void add (double value);
    virtual void add (long double value);

private:
    class Item;
    class Proxy;
};


class PropertyStream::Item : public List <Item>::Node
{
public:
    explicit Item (Source* source);
    Source& source() const;
    Source* operator-> () const;
    Source& operator* () const;
private:
    Source* m_source;
};


class PropertyStream::Proxy
{
private:
    Map const* m_map;
    std::string m_key;
    std::ostringstream mutable m_ostream;

public:
    Proxy (Map const& map, std::string const& key);
    Proxy (Proxy const& other);
    ~Proxy ();

    template <typename Value>
    Proxy& operator= (Value value);

    std::ostream& operator<< (std::ostream& manip (std::ostream&)) const;

    template <typename T>
    std::ostream& operator<< (T const& t) const
    {
        return m_ostream << t;
    }
};


class PropertyStream::Map
{
private:
    PropertyStream& m_stream;

public:
    explicit Map (PropertyStream& stream);
    explicit Map (Set& parent);
    Map (std::string const& key, Map& parent);
    Map (std::string const& key, PropertyStream& stream);
    ~Map ();

    Map(Map const&) = delete;
    Map& operator= (Map const&) = delete;

    PropertyStream& stream();
    PropertyStream const& stream() const;

    template <typename Value>
    void add (std::string const& key, Value value) const
    {
        m_stream.add (key, value);
    }

    template <typename Key, typename Value>
    void add (Key key, Value value) const
    {
        std::stringstream ss;
        ss << key;
        add (ss.str(), value);
    }

    Proxy operator[] (std::string const& key);

    Proxy operator[] (char const* key)
        { return Proxy (*this, key); }

    template <typename Key>
    Proxy operator[] (Key key) const
    {
        std::stringstream ss;
        ss << key;
        return Proxy (*this, ss.str());
    }
};


template <typename Value>
PropertyStream::Proxy& PropertyStream::Proxy::operator= (Value value)
{
    m_map->add (m_key, value);
    return *this;
}


class PropertyStream::Set
{
private:
    PropertyStream& m_stream;

public:
    Set (std::string const& key, Map& map);
    Set (std::string const& key, PropertyStream& stream);
    ~Set ();

    Set (Set const&) = delete;
    Set& operator= (Set const&) = delete;

    PropertyStream& stream();
    PropertyStream const& stream() const;

    template <typename Value>
    void add (Value value) const
        { m_stream.add (value); }
};



class PropertyStream::Source
{
private:
    std::string const m_name;
    std::recursive_mutex lock_;
    Item item_;
    Source* parent_;
    List <Item> children_;

public:
    explicit Source (std::string const& name);
    virtual ~Source ();

    Source (Source const&) = delete;
    Source& operator= (Source const&) = delete;

    
    std::string const& name() const;

    
    void add (Source& source);

    
    template <class Derived>
    Derived* add (Derived* child)
    {
        add (*static_cast <Source*>(child));
        return child;
    }

    
    void remove (Source& child);

    
    void removeAll ();

    
    void write_one  (PropertyStream& stream);

    
    void write      (PropertyStream& stream);

    
    void write      (PropertyStream& stream, std::string const& path);

    
    std::pair <Source*, bool> find (std::string path);

    Source* find_one_deep (std::string const& name);
    PropertyStream::Source* find_path(std::string path);
    PropertyStream::Source* find_one(std::string const& name);

    static bool peel_leading_slash (std::string* path);
    static bool peel_trailing_slashstar (std::string* path);
    static std::string peel_name(std::string* path);



    
    virtual void onWrite (Map&);
};

}

#endif








