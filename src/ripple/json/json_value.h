

#ifndef RIPPLE_JSON_JSON_VALUE_H_INCLUDED
#define RIPPLE_JSON_JSON_VALUE_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>


namespace Json
{


enum ValueType
{
    nullValue = 0, 
    intValue,      
    uintValue,     
    realValue,     
    stringValue,   
    booleanValue,  
    arrayValue,    
    objectValue    
};

enum CommentPlacement
{
    commentBefore = 0,        
    commentAfterOnSameLine,   
    commentAfter,             
    numberOfCommentPlacement
};


class StaticString
{
public:
    constexpr explicit StaticString ( const char* czstring )
        : str_ ( czstring )
    {
    }

    constexpr operator const char* () const
    {
        return str_;
    }

    constexpr const char* c_str () const
    {
        return str_;
    }

private:
    const char* str_;
};

inline bool operator== (StaticString x, StaticString y)
{
    return strcmp (x.c_str(), y.c_str()) == 0;
}

inline bool operator!= (StaticString x, StaticString y)
{
    return ! (x == y);
}

inline bool operator== (std::string const& x, StaticString y)
{
    return strcmp(x.c_str(), y.c_str()) == 0;
}

inline bool operator!= (std::string const& x, StaticString y)
{
    return ! (x == y);
}

inline bool operator== (StaticString x, std::string const& y)
{
    return y == x;
}

inline bool operator!= (StaticString x, std::string const& y)
{
    return ! (y == x);
}


class Value
{
    friend class ValueIteratorBase;

public:
    using Members = std::vector<std::string>;
    using iterator = ValueIterator;
    using const_iterator = ValueConstIterator;
    using UInt = Json::UInt;
    using Int = Json::Int;
    using ArrayIndex = UInt;

    static const Value null;
    static const Int minInt;
    static const Int maxInt;
    static const UInt maxUInt;

private:
    class CZString
    {
    public:
        enum DuplicationPolicy
        {
            noDuplication = 0,
            duplicate,
            duplicateOnCopy
        };
        CZString ( int index );
        CZString ( const char* cstr, DuplicationPolicy allocate );
        CZString ( const CZString& other );
        ~CZString ();
        CZString& operator = ( const CZString& other );
        bool operator< ( const CZString& other ) const;
        bool operator== ( const CZString& other ) const;
        int index () const;
        const char* c_str () const;
        bool isStaticString () const;
    private:
        void swap ( CZString& other ) noexcept;
        const char* cstr_;
        int index_;
    };

public:
    using ObjectValues = std::map<CZString, Value>;

public:
    
    Value ( ValueType type = nullValue );
    Value ( Int value );
    Value ( UInt value );
    Value ( double value );
    Value ( const char* value );
    Value ( const char* beginValue, const char* endValue );
    
    Value ( const StaticString& value );
    Value ( std::string const& value );
    Value ( bool value );
    Value ( const Value& other );
    ~Value ();

    Value& operator= ( Value const& other );
    Value& operator= ( Value&& other );

    Value ( Value&& other ) noexcept;

    void swap ( Value& other ) noexcept;

    ValueType type () const;

    const char* asCString () const;
    
    std::string asString () const;
    Int asInt () const;
    UInt asUInt () const;
    double asDouble () const;
    bool asBool () const;

    
    bool isNull () const;
    bool isBool () const;
    bool isInt () const;
    bool isUInt () const;
    bool isIntegral () const;
    bool isDouble () const;
    bool isNumeric () const;
    bool isString () const;
    bool isArray() const;
    bool isArrayOrNull () const;
    bool isObject() const;
    bool isObjectOrNull () const;

    bool isConvertibleTo ( ValueType other ) const;

    UInt size () const;

    
    explicit
    operator bool() const;

    void clear ();

    void resize ( UInt size );

    Value& operator[] ( UInt index );
    const Value& operator[] ( UInt index ) const;
    Value get ( UInt index,
                const Value& defaultValue ) const;
    bool isValidIndex ( UInt index ) const;
    Value& append ( const Value& value );

    Value& operator[] ( const char* key );
    const Value& operator[] ( const char* key ) const;
    Value& operator[] ( std::string const& key );
    const Value& operator[] ( std::string const& key ) const;
    
    Value& operator[] ( const StaticString& key );

    Value get ( const char* key,
                const Value& defaultValue ) const;
    Value get ( std::string const& key,
                const Value& defaultValue ) const;

    Value removeMember ( const char* key );
    Value removeMember ( std::string const& key );

    bool isMember ( const char* key ) const;
    bool isMember ( std::string const& key ) const;

    Members getMemberNames () const;

    bool hasComment ( CommentPlacement placement ) const;
    std::string getComment ( CommentPlacement placement ) const;

    std::string toStyledString () const;

    const_iterator begin () const;
    const_iterator end () const;

    iterator begin ();
    iterator end ();

    friend bool operator== (const Value&, const Value&);
    friend bool operator< (const Value&, const Value&);

private:
    Value& resolveReference ( const char* key,
                              bool isStatic );

private:
    union ValueHolder
    {
        Int int_;
        UInt uint_;
        double real_;
        bool bool_;
        char* string_;
        ObjectValues* map_ {nullptr};
    } value_;
    ValueType type_ : 8;
    int allocated_ : 1;     
};

bool operator== (const Value&, const Value&);

inline
bool operator!= (const Value& x, const Value& y)
{
    return ! (x == y);
}

bool operator< (const Value&, const Value&);

inline
bool operator<= (const Value& x, const Value& y)
{
    return ! (y < x);
}

inline
bool operator> (const Value& x, const Value& y)
{
    return y < x;
}

inline
bool operator>= (const Value& x, const Value& y)
{
    return ! (x < y);
}


class ValueAllocator
{
public:
    enum { unknown = (unsigned) - 1 };

    virtual ~ValueAllocator () = default;

    virtual char* makeMemberName ( const char* memberName ) = 0;
    virtual void releaseMemberName ( char* memberName ) = 0;
    virtual char* duplicateStringValue ( const char* value,
                                         unsigned int length = unknown ) = 0;
    virtual void releaseStringValue ( char* value ) = 0;
};


class ValueIteratorBase
{
public:
    using size_t = unsigned int;
    using difference_type = int;
    using SelfType = ValueIteratorBase;

    ValueIteratorBase ();

    explicit ValueIteratorBase ( const Value::ObjectValues::iterator& current );

    bool operator == ( const SelfType& other ) const
    {
        return isEqual ( other );
    }

    bool operator != ( const SelfType& other ) const
    {
        return !isEqual ( other );
    }

    difference_type operator - ( const SelfType& other ) const
    {
        return computeDistance ( other );
    }

    Value key () const;

    UInt index () const;

    const char* memberName () const;

protected:
    Value& deref () const;

    void increment ();

    void decrement ();

    difference_type computeDistance ( const SelfType& other ) const;

    bool isEqual ( const SelfType& other ) const;

    void copy ( const SelfType& other );

private:
    Value::ObjectValues::iterator current_;
    bool isNull_;
};


class ValueConstIterator : public ValueIteratorBase
{
    friend class Value;
public:
    using size_t = unsigned int;
    using difference_type = int;
    using reference = const Value&;
    using pointer = const Value*;
    using SelfType = ValueConstIterator;

    ValueConstIterator () = default;
private:
    
    explicit ValueConstIterator ( const Value::ObjectValues::iterator& current );
public:
    SelfType& operator = ( const ValueIteratorBase& other );

    SelfType operator++ ( int )
    {
        SelfType temp ( *this );
        ++*this;
        return temp;
    }

    SelfType operator-- ( int )
    {
        SelfType temp ( *this );
        --*this;
        return temp;
    }

    SelfType& operator-- ()
    {
        decrement ();
        return *this;
    }

    SelfType& operator++ ()
    {
        increment ();
        return *this;
    }

    reference operator * () const
    {
        return deref ();
    }
};



class ValueIterator : public ValueIteratorBase
{
    friend class Value;
public:
    using size_t = unsigned int;
    using difference_type = int;
    using reference = Value&;
    using pointer = Value*;
    using SelfType = ValueIterator;

    ValueIterator () = default;
    ValueIterator ( const ValueConstIterator& other );
    ValueIterator ( const ValueIterator& other );
private:
    
    explicit ValueIterator ( const Value::ObjectValues::iterator& current );
public:

    SelfType& operator = ( const SelfType& other );

    SelfType operator++ ( int )
    {
        SelfType temp ( *this );
        ++*this;
        return temp;
    }

    SelfType operator-- ( int )
    {
        SelfType temp ( *this );
        --*this;
        return temp;
    }

    SelfType& operator-- ()
    {
        decrement ();
        return *this;
    }

    SelfType& operator++ ()
    {
        increment ();
        return *this;
    }

    reference operator * () const
    {
        return deref ();
    }
};

} 


#endif 








