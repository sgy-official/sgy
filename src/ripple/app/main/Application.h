

#ifndef RIPPLE_APP_MAIN_APPLICATION_H_INCLUDED
#define RIPPLE_APP_MAIN_APPLICATION_H_INCLUDED

#include <ripple/shamap/FullBelowCache.h>
#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/basics/TaggedCache.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>

namespace ripple {

namespace unl { class Manager; }
namespace Resource { class Manager; }
namespace NodeStore { class Database; class DatabaseShard; }
namespace perf { class PerfLog; }

class AmendmentTable;
class CachedSLEs;
class CollectorManager;
class Family;
class HashRouter;
class Logs;
class LoadFeeTrack;
class JobQueue;
class InboundLedgers;
class InboundTransactions;
class AcceptedLedger;
class LedgerMaster;
class LoadManager;
class ManifestCache;
class NetworkOPs;
class OpenLedger;
class OrderBookDB;
class Overlay;
class PathRequests;
class PendingSaves;
class PublicKey;
class SecretKey;
class AccountIDCache;
class STLedgerEntry;
class TimeKeeper;
class TransactionMaster;
class TxQ;

class ValidatorList;
class ValidatorSite;
class Cluster;

class DatabaseCon;
class SHAMapStore;

using NodeCache     = TaggedCache <SHAMapHash, Blob>;

template <class Adaptor>
class Validations;
class RCLValidationsAdaptor;
using RCLValidations = Validations<RCLValidationsAdaptor>;

class Application : public beast::PropertyStream::Source
{
public:
    
    using MutexType = std::recursive_mutex;
    virtual MutexType& getMasterMutex () = 0;

public:
    Application ();

    virtual ~Application () = default;

    virtual bool setup() = 0;
    virtual void doStart(bool withTimers) = 0;
    virtual void run() = 0;
    virtual bool isShutdown () = 0;
    virtual void signalStop () = 0;
    virtual bool checkSigs() const = 0;
    virtual void checkSigs(bool) = 0;


    virtual Logs&                   logs() = 0;
    virtual Config&                 config() = 0;

    virtual
    boost::asio::io_service&
    getIOService () = 0;

    virtual CollectorManager&           getCollectorManager () = 0;
    virtual Family&                     family() = 0;
    virtual Family*                     shardFamily() = 0;
    virtual TimeKeeper&                 timeKeeper() = 0;
    virtual JobQueue&                   getJobQueue () = 0;
    virtual NodeCache&                  getTempNodeCache () = 0;
    virtual CachedSLEs&                 cachedSLEs() = 0;
    virtual AmendmentTable&             getAmendmentTable() = 0;
    virtual HashRouter&                 getHashRouter () = 0;
    virtual LoadFeeTrack&               getFeeTrack () = 0;
    virtual LoadManager&                getLoadManager () = 0;
    virtual Overlay&                    overlay () = 0;
    virtual TxQ&                        getTxQ() = 0;
    virtual ValidatorList&              validators () = 0;
    virtual ValidatorSite&              validatorSites () = 0;
    virtual ManifestCache&              validatorManifests () = 0;
    virtual ManifestCache&              publisherManifests () = 0;
    virtual Cluster&                    cluster () = 0;
    virtual RCLValidations&             getValidations () = 0;
    virtual NodeStore::Database&        getNodeStore () = 0;
    virtual NodeStore::DatabaseShard*   getShardStore() = 0;
    virtual InboundLedgers&             getInboundLedgers () = 0;
    virtual InboundTransactions&        getInboundTransactions () = 0;

    virtual
    TaggedCache <uint256, AcceptedLedger>&
    getAcceptedLedgerCache () = 0;

    virtual LedgerMaster&           getLedgerMaster () = 0;
    virtual NetworkOPs&             getOPs () = 0;
    virtual OrderBookDB&            getOrderBookDB () = 0;
    virtual TransactionMaster&      getMasterTransaction () = 0;
    virtual perf::PerfLog&          getPerfLog () = 0;

    virtual
    std::pair<PublicKey, SecretKey> const&
    nodeIdentity () = 0;

    virtual
    PublicKey const &
    getValidationPublicKey() const  = 0;

    virtual Resource::Manager&      getResourceManager () = 0;
    virtual PathRequests&           getPathRequests () = 0;
    virtual SHAMapStore&            getSHAMapStore () = 0;
    virtual PendingSaves&           pendingSaves() = 0;
    virtual AccountIDCache const&   accountIDCache() const = 0;
    virtual OpenLedger&             openLedger() = 0;
    virtual OpenLedger const&       openLedger() const = 0;
    virtual DatabaseCon&            getTxnDB () = 0;
    virtual DatabaseCon&            getLedgerDB () = 0;

    virtual
    std::chrono::milliseconds
    getIOLatency () = 0;

    virtual bool serverOkay (std::string& reason) = 0;

    virtual beast::Journal journal (std::string const& name) = 0;

    
    virtual int fdlimit () const = 0;

    
    virtual DatabaseCon& getWalletDB () = 0;

    
    virtual LedgerIndex getMaxDisallowedLedger() = 0;
};

std::unique_ptr <Application>
make_Application(
    std::unique_ptr<Config> config,
    std::unique_ptr<Logs> logs,
    std::unique_ptr<TimeKeeper> timeKeeper);

}

#endif








