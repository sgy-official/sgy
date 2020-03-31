

#ifndef RIPPLE_LEDGER_APPLYVIEW_H_INCLUDED
#define RIPPLE_LEDGER_APPLYVIEW_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <boost/optional.hpp>

namespace ripple {

enum ApplyFlags
    : std::uint32_t
{
    tapNONE             = 0x00,

    tapRETRY            = 0x20,

    tapPREFER_QUEUE     = 0x40,

    tapUNLIMITED        = 0x400,
};

constexpr
ApplyFlags
operator|(ApplyFlags const& lhs,
    ApplyFlags const& rhs)
{
    return safe_cast<ApplyFlags>(
        safe_cast<std::underlying_type_t<ApplyFlags>>(lhs) |
            safe_cast<std::underlying_type_t<ApplyFlags>>(rhs));
}

static_assert((tapPREFER_QUEUE | tapRETRY) == safe_cast<ApplyFlags>(0x60u),
    "ApplyFlags operator |");
static_assert((tapRETRY | tapPREFER_QUEUE) == safe_cast<ApplyFlags>(0x60u),
    "ApplyFlags operator |");

constexpr
ApplyFlags
operator&(ApplyFlags const& lhs,
    ApplyFlags const& rhs)
{
    return safe_cast<ApplyFlags>(
        safe_cast<std::underlying_type_t<ApplyFlags>>(lhs) &
            safe_cast<std::underlying_type_t<ApplyFlags>>(rhs));
}

static_assert((tapPREFER_QUEUE & tapRETRY) == tapNONE,
    "ApplyFlags operator &");
static_assert((tapRETRY & tapPREFER_QUEUE) == tapNONE,
    "ApplyFlags operator &");

constexpr
ApplyFlags
operator~(ApplyFlags const& flags)
{
    return safe_cast<ApplyFlags>(
        ~safe_cast<std::underlying_type_t<ApplyFlags>>(flags));
}

static_assert(~tapRETRY == safe_cast<ApplyFlags>(0xFFFFFFDFu),
    "ApplyFlags operator ~");

inline
ApplyFlags
operator|=(ApplyFlags & lhs,
    ApplyFlags const& rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline
ApplyFlags
operator&=(ApplyFlags& lhs,
    ApplyFlags const& rhs)
{
    lhs = lhs & rhs;
    return lhs;
}



class ApplyView
    : public ReadView
{
private:
    
    boost::optional<std::uint64_t>
    dirAdd (
        bool preserveOrder,
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe);

public:
    ApplyView () = default;

    
    virtual
    ApplyFlags
    flags() const = 0;

    
    virtual
    std::shared_ptr<SLE>
    peek (Keylet const& k) = 0;

    
    virtual
    void
    erase (std::shared_ptr<SLE> const& sle) = 0;

    
    virtual
    void
    insert (std::shared_ptr<SLE> const& sle) = 0;

    
    
    virtual
    void
    update (std::shared_ptr<SLE> const& sle) = 0;


    virtual void
    creditHook (AccountID const& from,
        AccountID const& to,
        STAmount const& amount,
        STAmount const& preCreditBalance)
    {
    }

    virtual
    void adjustOwnerCountHook (AccountID const& account,
        std::uint32_t cur, std::uint32_t next)
    {}

    
    
    boost::optional<std::uint64_t>
    dirAppend (
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAdd (true, directory, key, describe);
    }

    boost::optional<std::uint64_t>
    dirAppend (
        Keylet const& directory,
        Keylet const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAppend (directory, key.key, describe);
    }
    

    
    
    boost::optional<std::uint64_t>
    dirInsert (
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAdd (false, directory, key, describe);
    }

    boost::optional<std::uint64_t>
    dirInsert (
        Keylet const& directory,
        Keylet const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirInsert (directory, key.key, describe);
    }
    

    
    
    bool
    dirRemove (
        Keylet const& directory,
        std::uint64_t page,
        uint256 const& key,
        bool keepRoot);

    bool
    dirRemove (
        Keylet const& directory,
        std::uint64_t page,
        Keylet const& key,
        bool keepRoot)
    {
        return dirRemove (directory, page, key.key, keepRoot);
    }
    
};

} 

#endif








