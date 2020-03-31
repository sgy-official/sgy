

#ifndef RIPPLE_JSON_OBJECT_H_INCLUDED
#define RIPPLE_JSON_OBJECT_H_INCLUDED

#include <ripple/json/Writer.h>
#include <memory>

namespace Json {



class Collection
{
public:
    Collection (Collection&& c) noexcept;
    Collection& operator= (Collection&& c) noexcept;
    Collection() = delete;

    ~Collection();

protected:
    Collection (Collection* parent, Writer*);
    void checkWritable (std::string const& label);

    Collection* parent_;
    Writer* writer_;
    bool enabled_;
};

class Array;



class Object : protected Collection
{
public:
    
    class Root;

    
    template <typename Scalar>
    void set (std::string const& key, Scalar const&);

    void set (std::string const& key, Json::Value const&);

    class Proxy;

    Proxy operator[] (std::string const& key);
    Proxy operator[] (Json::StaticString const& key);

    
    Object setObject (std::string const& key);

    
    Array setArray (std::string const& key);

protected:
    friend class Array;
    Object (Collection* parent, Writer* w) : Collection (parent, w) {}
};

class Object::Root : public Object
{
  public:
    
    Root (Writer&);
};



class Array : private Collection
{
public:
    
    template <typename Scalar>
    void append (Scalar const&);

    
    void append (Json::Value const&);

    
    Object appendObject ();

    
    Array appendArray ();

  protected:
    friend class Object;
    Array (Collection* parent, Writer* w) : Collection (parent, w) {}
};




Json::Value& setArray (Json::Value&, Json::StaticString const& key);


Array setArray (Object&, Json::StaticString const& key);



Json::Value& addObject (Json::Value&, Json::StaticString const& key);


Object addObject (Object&, Json::StaticString const& key);



Json::Value& appendArray (Json::Value&);


Array appendArray (Array&);



Json::Value& appendObject (Json::Value&);


Object appendObject (Array&);



void copyFrom (Json::Value& to, Json::Value const& from);


void copyFrom (Object& to, Json::Value const& from);



class WriterObject
{
public:
    WriterObject (Output const& output)
            : writer_ (std::make_unique<Writer> (output)),
              object_ (std::make_unique<Object::Root> (*writer_))
    {
    }

    WriterObject (WriterObject&& other) = default;

    Object* operator->()
    {
        return object_.get();
    }

    Object& operator*()
    {
        return *object_;
    }

private:
    std::unique_ptr <Writer> writer_;
    std::unique_ptr<Object::Root> object_;
};

WriterObject stringWriterObject (std::string&);


class Object::Proxy
{
private:
    Object& object_;
    std::string const key_;

public:
    Proxy (Object& object, std::string const& key);

    template <class T>
    void operator= (T const& t)
    {
        object_.set (key_, t);
    }
};


template <typename Scalar>
void Array::append (Scalar const& value)
{
    checkWritable ("append");
    if (writer_)
        writer_->append (value);
}

template <typename Scalar>
void Object::set (std::string const& key, Scalar const& value)
{
    checkWritable ("set");
    if (writer_)
        writer_->set (key, value);
}

inline
Json::Value& setArray (Json::Value& json, Json::StaticString const& key)
{
    return (json[key] = Json::arrayValue);
}

inline
Array setArray (Object& json, Json::StaticString const& key)
{
    return json.setArray (std::string (key));
}

inline
Json::Value& addObject (Json::Value& json, Json::StaticString const& key)
{
    return (json[key] = Json::objectValue);
}

inline
Object addObject (Object& object, Json::StaticString const& key)
{
    return object.setObject (std::string (key));
}

inline
Json::Value& appendArray (Json::Value& json)
{
    return json.append (Json::arrayValue);
}

inline
Array appendArray (Array& json)
{
    return json.appendArray ();
}

inline
Json::Value& appendObject (Json::Value& json)
{
    return json.append (Json::objectValue);
}

inline
Object appendObject (Array& json)
{
    return json.appendObject ();
}

} 

#endif








