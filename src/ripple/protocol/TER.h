

#ifndef RIPPLE_PROTOCOL_TER_H_INCLUDED
#define RIPPLE_PROTOCOL_TER_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/json/json_value.h>

#include <boost/optional.hpp>
#include <ostream>
#include <string>

namespace ripple {

using TERUnderlyingType = int;


enum TELcodes : TERUnderlyingType
{

    telLOCAL_ERROR  = -399,
    telBAD_DOMAIN,
    telBAD_PATH_COUNT,
    telBAD_PUBLIC_KEY,
    telFAILED_PROCESSING,
    telINSUF_FEE_P,
    telNO_DST_PARTIAL,
    telCAN_NOT_QUEUE,
    telCAN_NOT_QUEUE_BALANCE,
    telCAN_NOT_QUEUE_BLOCKS,
    telCAN_NOT_QUEUE_BLOCKED,
    telCAN_NOT_QUEUE_FEE,
    telCAN_NOT_QUEUE_FULL
};


enum TEMcodes : TERUnderlyingType
{

    temMALFORMED    = -299,

    temBAD_AMOUNT,
    temBAD_CURRENCY,
    temBAD_EXPIRATION,
    temBAD_FEE,
    temBAD_ISSUER,
    temBAD_LIMIT,
    temBAD_OFFER,
    temBAD_PATH,
    temBAD_PATH_LOOP,
    temBAD_REGKEY,
    temBAD_SEND_XRP_LIMIT,
    temBAD_SEND_XRP_MAX,
    temBAD_SEND_XRP_NO_DIRECT,
    temBAD_SEND_XRP_PARTIAL,
    temBAD_SEND_XRP_PATHS,
    temBAD_SEQUENCE,
    temBAD_SIGNATURE,
    temBAD_SRC_ACCOUNT,
    temBAD_TRANSFER_RATE,
    temDST_IS_SRC,
    temDST_NEEDED,
    temINVALID,
    temINVALID_FLAG,
    temREDUNDANT,
    temRIPPLE_EMPTY,
    temDISABLED,
    temBAD_SIGNER,
    temBAD_QUORUM,
    temBAD_WEIGHT,
    temBAD_TICK_SIZE,
    temINVALID_ACCOUNT_ID,
    temCANNOT_PREAUTH_SELF,

    temUNCERTAIN,
    temUNKNOWN
};


enum TEFcodes : TERUnderlyingType
{

    tefFAILURE      = -199,
    tefALREADY,
    tefBAD_ADD_AUTH,
    tefBAD_AUTH,
    tefBAD_LEDGER,
    tefCREATED,
    tefEXCEPTION,
    tefINTERNAL,
    tefNO_AUTH_REQUIRED,    
    tefPAST_SEQ,
    tefWRONG_PRIOR,
    tefMASTER_DISABLED,
    tefMAX_LEDGER,
    tefBAD_SIGNATURE,
    tefBAD_QUORUM,
    tefNOT_MULTI_SIGNING,
    tefBAD_AUTH_MASTER,
    tefINVARIANT_FAILED,
};


enum TERcodes : TERUnderlyingType
{

    terRETRY        = -99,
    terFUNDS_SPENT,      
    terINSUF_FEE_B,      
    terNO_ACCOUNT,       
    terNO_AUTH,          
    terNO_LINE,          
    terOWNERS,           
    terPRE_SEQ,          
    terLAST,             
    terNO_RIPPLE,        
    terQUEUED            
};


enum TEScodes : TERUnderlyingType
{

    tesSUCCESS      = 0
};


enum TECcodes : TERUnderlyingType
{

    tecCLAIM                    = 100,
    tecPATH_PARTIAL             = 101,
    tecUNFUNDED_ADD             = 102,
    tecUNFUNDED_OFFER           = 103,
    tecUNFUNDED_PAYMENT         = 104,
    tecFAILED_PROCESSING        = 105,
    tecDIR_FULL                 = 121,
    tecINSUF_RESERVE_LINE       = 122,
    tecINSUF_RESERVE_OFFER      = 123,
    tecNO_DST                   = 124,
    tecNO_DST_INSUF_XRP         = 125,
    tecNO_LINE_INSUF_RESERVE    = 126,
    tecNO_LINE_REDUNDANT        = 127,
    tecPATH_DRY                 = 128,
    tecUNFUNDED                 = 129,  
    tecNO_ALTERNATIVE_KEY       = 130,
    tecNO_REGULAR_KEY           = 131,
    tecOWNERS                   = 132,
    tecNO_ISSUER                = 133,
    tecNO_AUTH                  = 134,
    tecNO_LINE                  = 135,
    tecINSUFF_FEE               = 136,
    tecFROZEN                   = 137,
    tecNO_TARGET                = 138,
    tecNO_PERMISSION            = 139,
    tecNO_ENTRY                 = 140,
    tecINSUFFICIENT_RESERVE     = 141,
    tecNEED_MASTER_KEY          = 142,
    tecDST_TAG_NEEDED           = 143,
    tecINTERNAL                 = 144,
    tecOVERSIZE                 = 145,
    tecCRYPTOCONDITION_ERROR    = 146,
    tecINVARIANT_FAILED         = 147,
    tecEXPIRED                  = 148,
    tecDUPLICATE                = 149,
    tecKILLED                   = 150,
};


constexpr TERUnderlyingType TERtoInt (TELcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEMcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEFcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TERcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEScodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TECcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

template <template<typename> class Trait>
class TERSubset
{
    TERUnderlyingType code_;

public:
    constexpr TERSubset() : code_ (tesSUCCESS) { }
    constexpr TERSubset (TERSubset const& rhs) = default;
    constexpr TERSubset (TERSubset&& rhs) = default;
private:
    constexpr explicit TERSubset (int rhs) : code_ (rhs) { }
public:
    static constexpr TERSubset fromInt (int from)
    {
        return TERSubset (from);
    }

    template <typename T, typename = std::enable_if_t<Trait<T>::value>>
    constexpr TERSubset (T rhs)
    : code_ (TERtoInt (rhs))
    { }

    constexpr TERSubset& operator=(TERSubset const& rhs) = default;
    constexpr TERSubset& operator=(TERSubset&& rhs) = default;

    template <typename T>
    constexpr auto
    operator= (T rhs) -> std::enable_if_t<Trait<T>::value, TERSubset&>
    {
        code_ = TERtoInt (rhs);
        return *this;
    }

    explicit operator bool() const
    {
        return code_ != tesSUCCESS;
    }

    operator Json::Value() const
    {
        return Json::Value {code_};
    }

    friend std::ostream& operator<< (std::ostream& os, TERSubset const& rhs)
    {
        return os << rhs.code_;
    }

    friend constexpr TERUnderlyingType TERtoInt (TERSubset v)
    {
        return v.code_;
    }
};

template <typename L, typename R>
constexpr auto
operator== (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) == TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator!= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) != TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator< (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) < TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator<= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) <= TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator> (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) > TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator>= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) >= TERtoInt(rhs);
}



template <typename FROM> class CanCvtToNotTEC           : public std::false_type {};
template <>              class CanCvtToNotTEC<TELcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEMcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEFcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TERcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEScodes> : public std::true_type {};

using NotTEC = TERSubset<CanCvtToNotTEC>;


template <typename FROM> class CanCvtToTER           : public std::false_type {};
template <>              class CanCvtToTER<TELcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEMcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEFcodes> : public std::true_type {};
template <>              class CanCvtToTER<TERcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEScodes> : public std::true_type {};
template <>              class CanCvtToTER<TECcodes> : public std::true_type {};
template <>              class CanCvtToTER<NotTEC>   : public std::true_type {};

using TER = TERSubset<CanCvtToTER>;


inline bool isTelLocal(TER x)
{
    return ((x) >= telLOCAL_ERROR && (x) < temMALFORMED);
}

inline bool isTemMalformed(TER x)
{
    return ((x) >= temMALFORMED && (x) < tefFAILURE);
}

inline bool isTefFailure(TER x)
{
    return ((x) >= tefFAILURE && (x) < terRETRY);
}

inline bool isTerRetry(TER x)
{
    return ((x) >= terRETRY && (x) < tesSUCCESS);
}

inline bool isTesSuccess(TER x)
{
    return ((x) == tesSUCCESS);
}

inline bool isTecClaim(TER x)
{
    return ((x) >= tecCLAIM);
}

bool
transResultInfo (TER code, std::string& token, std::string& text);

std::string
transToken (TER code);

std::string
transHuman (TER code);

boost::optional<TER>
transCode(std::string const& token);

} 

#endif








