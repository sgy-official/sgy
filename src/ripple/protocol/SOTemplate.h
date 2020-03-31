

#ifndef RIPPLE_PROTOCOL_SOTEMPLATE_H_INCLUDED
#define RIPPLE_PROTOCOL_SOTEMPLATE_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SField.h>
#include <functional>
#include <initializer_list>
#include <memory>

namespace ripple {


enum SOEStyle
{
    soeINVALID  = -1,
    soeREQUIRED = 0,   
    soeOPTIONAL = 1,   
    soeDEFAULT  = 2,   
};



class SOElement
{
    std::reference_wrapper<SField const> sField_;
    SOEStyle                             style_;

public:
    SOElement (SField const& fieldName, SOEStyle style)
        : sField_ (fieldName)
        , style_ (style)
    {
        if (! sField_.get().isUseful())
            Throw<std::runtime_error> ("SField in SOElement must be useful.");
    }

    SField const& sField () const
    {
        return sField_.get();
    }

    SOEStyle style () const
    {
        return style_;
    }
};



class SOTemplate
{
public:
    SOTemplate(SOTemplate&& other) = default;
    SOTemplate& operator=(SOTemplate&& other) = default;

    
    SOTemplate (std::initializer_list<SOElement> uniqueFields,
        std::initializer_list<SOElement> commonFields = {});

    
    std::vector<SOElement>::const_iterator begin() const
    {
        return elements_.cbegin();
    }

    std::vector<SOElement>::const_iterator cbegin() const
    {
        return begin();
    }

    std::vector<SOElement>::const_iterator end() const
    {
        return elements_.cend();
    }

    std::vector<SOElement>::const_iterator cend() const
    {
        return end();
    }

    
    std::size_t size () const
    {
        return elements_.size ();
    }

    
    int getIndex (SField const&) const;

    SOEStyle
    style(SField const& sf) const
    {
        return elements_[indices_[sf.getNum()]].style();
    }

private:
    std::vector<SOElement> elements_;
    std::vector<int> indices_;              
};

} 

#endif








