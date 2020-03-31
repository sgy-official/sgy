

#ifndef RIPPLE_APP_MISC_VALIDATOR_KEYS_H_INCLUDED
#define RIPPLE_APP_MISC_VALIDATOR_KEYS_H_INCLUDED

#include <ripple/beast/utility/Journal.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/UintTypes.h>
#include <string>

namespace ripple {

class Config;


class ValidatorKeys
{
public:
    PublicKey publicKey;
    SecretKey secretKey;
    NodeID nodeID;
    std::string manifest;

    ValidatorKeys(Config const& config, beast::Journal j);

    bool configInvalid() const
    {
        return configInvalid_;
    }

private:
    bool configInvalid_ = false; 
};

}  

#endif








