

#ifndef RIPPLE_RPC_STATUS_H_INCLUDED
#define RIPPLE_RPC_STATUS_H_INCLUDED

#include <ripple/protocol/TER.h>
#include <ripple/protocol/ErrorCodes.h>
#include <cassert>

namespace ripple {
namespace RPC {


struct Status : public std::exception
{
public:
    enum class Type {none, TER, error_code_i};
    using Code = int;
    using Strings = std::vector <std::string>;

    static constexpr Code OK = 0;

    Status () = default;

    template <typename T,
        typename = std::enable_if_t<std::is_integral<T>::value>>
    Status (T code, Strings d = {})
        : type_ (Type::none), code_ (code), messages_ (std::move (d))
    {
    }

    Status (TER ter, Strings d = {})
        : type_ (Type::TER), code_ (TERtoInt (ter)), messages_ (std::move (d))
    {
    }

    Status (error_code_i e, Strings d = {})
        : type_ (Type::error_code_i), code_ (e), messages_ (std::move (d))
    {
    }

    Status (error_code_i e, std::string const& s)
        : type_ (Type::error_code_i), code_ (e), messages_ ({s})
    {
    }

    
    std::string codeString () const;

    
    operator bool() const
    {
        return code_ != OK;
    }

    
    bool operator !() const
    {
        return ! bool (*this);
    }

    
    TER toTER () const
    {
        assert (type_ == Type::TER);
        return TER::fromInt (code_);
    }

    
    error_code_i toErrorCode() const
    {
        assert (type_ == Type::error_code_i);
        return error_code_i (code_);
    }

    
    template <class Object>
    void inject (Object& object)
    {
        if (auto ec = toErrorCode())
        {
            if (messages_.empty())
                inject_error (ec, object);
            else
                inject_error (ec, message(), object);
        }
    }

    Strings const& messages() const
    {
        return messages_;
    }

    
    std::string message() const;

    Type type() const
    {
        return type_;
    }

    std::string toString() const;

    
    void fillJson(Json::Value&);

private:
    Type type_ = Type::none;
    Code code_ = OK;
    Strings messages_;
};

} 
} 

#endif








