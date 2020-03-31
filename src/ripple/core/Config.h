

#ifndef RIPPLE_CORE_CONFIG_H_INCLUDED
#define RIPPLE_CORE_CONFIG_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/SystemParameters.h> 
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/beast/core/string.hpp>
#include <ripple/beast/utility/Journal.h>
#include <boost/filesystem.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace ripple {

class Rules;


enum SizedItemName
{
    siSweepInterval,
    siNodeCacheSize,
    siNodeCacheAge,
    siTreeCacheSize,
    siTreeCacheAge,
    siSLECacheSize,
    siSLECacheAge,
    siLedgerSize,
    siLedgerAge,
    siLedgerFetch,
    siHashNodeDBCache,
    siTxnDBCache,
    siLgrDBCache,
};

struct SizedItem
{
    SizedItemName   item;
    int             sizes[5];
};

class Config : public BasicConfig
{
public:
    static char const* const configFileName;
    static char const* const databaseDirName;
    static char const* const validatorsFileName;

    
    boost::filesystem::path getDebugLogFile () const;

private:
    boost::filesystem::path CONFIG_FILE;
public:
    boost::filesystem::path CONFIG_DIR;
private:
    boost::filesystem::path DEBUG_LOGFILE;

    void load ();
    beast::Journal j_;

    bool QUIET = false;          
    bool SILENT = false;         
    
    bool                        RUN_STANDALONE = false;

    
    bool signingEnabled_ = false;

public:
    bool doImport = false;
    bool nodeToShard = false;
    bool validateShards = false;
    bool ELB_SUPPORT = false;

    std::vector<std::string>    IPS;                    
    std::vector<std::string>    IPS_FIXED;              
    std::vector<std::string>    SNTP_SERVERS;           

    enum StartUpType
    {
        FRESH,
        NORMAL,
        LOAD,
        LOAD_FILE,
        REPLAY,
        NETWORK
    };
    StartUpType                 START_UP = NORMAL;

    bool                        START_VALID = false;

    std::string                 START_LEDGER;

    int const                   TRANSACTION_FEE_BASE = 10;   

    std::size_t                 NETWORK_QUORUM = 1;

    bool                        PEER_PRIVATE = false;           
    std::size_t                 PEERS_MAX = 0;

    std::chrono::seconds        WEBSOCKET_PING_FREQ = std::chrono::minutes {5};

    int                         PATH_SEARCH_OLD = 7;
    int                         PATH_SEARCH = 7;
    int                         PATH_SEARCH_FAST = 2;
    int                         PATH_SEARCH_MAX = 10;

    boost::optional<std::size_t> VALIDATION_QUORUM;     

    std::uint64_t                      FEE_DEFAULT = 10;
    std::uint64_t                      FEE_ACCOUNT_RESERVE = 0*SYSTEM_CURRENCY_PARTS;
    std::uint64_t                      FEE_OWNER_RESERVE = 0*SYSTEM_CURRENCY_PARTS;
    std::uint64_t                      FEE_OFFER = 10;

    std::uint32_t                      LEDGER_HISTORY = 256;
    std::uint32_t                      FETCH_DEPTH = 1000000000;
    int                         NODE_SIZE = 0;

    bool                        SSL_VERIFY = true;
    std::string                 SSL_VERIFY_FILE;
    std::string                 SSL_VERIFY_DIR;

    std::size_t                 WORKERS = 0;

    boost::optional<beast::IP::Endpoint> rpc_ip;

    std::unordered_set<uint256, beast::uhash<>> features;

public:
    Config()
    : j_ {beast::Journal::getNullSink()}
    { }

    int getSize (SizedItemName) const;
    
    void setup (std::string const& strConf, bool bQuiet,
        bool bSilent, bool bStandalone);
    void setupControl (bool bQuiet,
        bool bSilent, bool bStandalone);

    
    void loadFromString (std::string const& fileContents);

    bool quiet() const { return QUIET; }
    bool silent() const { return SILENT; }
    bool standalone() const { return RUN_STANDALONE; }

    bool canSign() const { return signingEnabled_; }
};

} 

#endif








