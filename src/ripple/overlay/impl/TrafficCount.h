

#ifndef RIPPLE_OVERLAY_TRAFFIC_H_INCLUDED
#define RIPPLE_OVERLAY_TRAFFIC_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/messages.h>

#include <array>
#include <atomic>
#include <cstdint>

namespace ripple {

class TrafficCount
{
public:
    class TrafficStats
    {
    public:
        std::string const name;

        std::atomic<std::uint64_t> bytesIn {0};
        std::atomic<std::uint64_t> bytesOut {0};
        std::atomic<std::uint64_t> messagesIn {0};
        std::atomic<std::uint64_t> messagesOut {0};

        TrafficStats(char const* n)
            : name (n)
        {
        }

        TrafficStats(TrafficStats const& ts)
            : name (ts.name)
            , bytesIn (ts.bytesIn.load())
            , bytesOut (ts.bytesOut.load())
            , messagesIn (ts.messagesIn.load())
            , messagesOut (ts.messagesOut.load())
        {
        }

        operator bool () const
        {
            return messagesIn || messagesOut;
        }
    };

    enum category : std::size_t
    {
        base,           

        cluster,        
        overlay,        
        manifests,      
        transaction,
        proposal,
        validation,
        shards,         

        get_set,        
        share_set,      

        ld_tsc_get,
        ld_tsc_share,

        ld_txn_get,
        ld_txn_share,

        ld_asn_get,
        ld_asn_share,

        ld_get,
        ld_share,

        gl_tsc_share,
        gl_tsc_get,

        gl_txn_share,
        gl_txn_get,

        gl_asn_share,
        gl_asn_get,

        gl_share,
        gl_get,

        share_hash_ledger,
        get_hash_ledger,

        share_hash_tx,
        get_hash_tx,

        share_hash_txnode,
        get_hash_txnode,

        share_hash_asnode,
        get_hash_asnode,

        share_cas_object,
        get_cas_object,

        share_fetch_pack,
        get_fetch_pack,

        share_hash,
        get_hash,

        unknown         
    };

    
    static category categorize (
        ::google::protobuf::Message const& message,
        int type, bool inbound);

    
    void addCount (category cat, bool inbound, int bytes)
    {
        assert (cat <= category::unknown);

        if (inbound)
        {
            counts_[cat].bytesIn += bytes;
            ++counts_[cat].messagesIn;
        }
        else
        {
            counts_[cat].bytesOut += bytes;
            ++counts_[cat].messagesOut;
        }
    }

    TrafficCount() = default;

    
    auto
    getCounts () const
    {
        return counts_;
    }

protected:
    std::array<TrafficStats, category::unknown + 1> counts_
    {{
        { "overhead" },                                           
        { "overhead: cluster" },                                  
        { "overhead: overlay" },                                  
        { "overhead: manifest" },                                 
        { "transactions" },                                       
        { "proposals" },                                          
        { "validations" },                                        
        { "shards" },                                             
        { "set (get)" },                                          
        { "set (share)" },                                        
        { "ledger data: Transaction Set candidate (get)" },       
        { "ledger data: Transaction Set candidate (share)" },     
        { "ledger data: Transaction Node (get)" },                
        { "ledger data: Transaction Node (share)" },              
        { "ledger data: Account State Node (get)" },              
        { "ledger data: Account State Node (share)" },            
        { "ledger data (get)" },                                  
        { "ledger data (share)" },                                
        { "ledger: Transaction Set candidate (share)" },          
        { "ledger: Transaction Set candidate (get)" },            
        { "ledger: Transaction node (share)" },                   
        { "ledger: Transaction node (get)" },                     
        { "ledger: Account State node (share)" },                 
        { "ledger: Account State node (get)" },                   
        { "ledger (share)" },                                     
        { "ledger (get)" },                                       
        { "getobject: Ledger (share)" },                          
        { "getobject: Ledger (get)" },                            
        { "getobject: Transaction (share)" },                     
        { "getobject: Transaction (get)" },                       
        { "getobject: Transaction node (share)" },                
        { "getobject: Transaction node (get)" },                  
        { "getobject: Account State node (share)" },              
        { "getobject: Account State node (get)" },                
        { "getobject: CAS (share)" },                             
        { "getobject: CAS (get)" },                               
        { "getobject: Fetch Pack (share)" },                      
        { "getobject: Fetch Pack (get)" },                        
        { "getobject (share)" },                                  
        { "getobject (get)" },                                    
        { "unknown" }                                             
    }};
};

}
#endif








