

#ifndef RIPPLE_APP_MAIN_NODEIDENTITY_H_INCLUDED
#define RIPPLE_APP_MAIN_NODEIDENTITY_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <utility>

namespace ripple {


std::pair<PublicKey, SecretKey>
loadNodeIdentity (Application& app);

} 

#endif








