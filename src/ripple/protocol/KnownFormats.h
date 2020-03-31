

#ifndef RIPPLE_PROTOCOL_KNOWNFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_KNOWNFORMATS_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SOTemplate.h>
#include <boost/container/flat_map.hpp>
#include <algorithm>
#include <forward_list>

namespace ripple {


template <class KeyType>
class KnownFormats
{
public:
    
    class Item
    {
    public:
        Item (char const* name,
            KeyType type,
            std::initializer_list<SOElement> uniqueFields,
            std::initializer_list<SOElement> commonFields)
            : soTemplate_ (uniqueFields, commonFields)
            , name_ (name)
            , type_ (type)
        {
            static_assert (
                std::is_enum<KeyType>::value ||
                std::is_integral<KeyType>::value,
                "KnownFormats KeyType must be integral or enum.");
        }

        
        std::string const& getName () const
        {
            return name_;
        }

        
        KeyType getType () const
        {
            return type_;
        }

        SOTemplate const& getSOTemplate() const
        {
            return soTemplate_;
        }

    private:
        SOTemplate soTemplate_;
        std::string const name_;
        KeyType const type_;
    };

    
    KnownFormats () = default;

    
    virtual ~KnownFormats () = default;
    KnownFormats(KnownFormats const&) = delete;
    KnownFormats& operator=(KnownFormats const&) = delete;

    
    KeyType findTypeByName (std::string const& name) const
    {
        Item const* const result = findByName (name);

        if (result != nullptr)
            return result->getType ();
        Throw<std::runtime_error> ("Unknown format name");
        return {}; 
    }

    
    Item const* findByType (KeyType type) const
    {
        auto const itr = types_.find (type);
        if (itr == types_.end())
            return nullptr;
        return itr->second;
    }

protected:
    
    Item const* findByName (std::string const& name) const
    {
        auto const itr = names_.find (name);
        if (itr == names_.end())
            return nullptr;
        return itr->second;
    }

    
    Item const& add (char const* name, KeyType type,
        std::initializer_list<SOElement> uniqueFields,
        std::initializer_list<SOElement> commonFields = {})
    {
        formats_.emplace_front (name, type, uniqueFields, commonFields);
        Item const& item {formats_.front()};

        names_[name] = &item;
        types_[type] = &item;

        return item;
    }

private:
    std::forward_list<Item> formats_;

    boost::container::flat_map<std::string, Item const*> names_;
    boost::container::flat_map<KeyType, Item const*> types_;
};

} 

#endif








