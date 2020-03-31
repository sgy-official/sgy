

#ifndef RIPPLE_CORE_JOB_H_INCLUDED
#define RIPPLE_CORE_JOB_H_INCLUDED

#include <ripple/core/LoadMonitor.h>
#include <functional>

#include <functional>

namespace ripple {


enum JobType
{
    jtINVALID = -1,


    jtPACK,          
    jtPUBOLDLEDGER,  
    jtVALIDATION_ut, 
    jtTRANSACTION_l, 
    jtLEDGER_REQ,    
    jtPROPOSAL_ut,   
    jtLEDGER_DATA,   
    jtCLIENT,        
    jtRPC,           
    jtUPDATE_PF,     
    jtTRANSACTION,   
    jtBATCH,         
    jtADVANCE,       
    jtPUBLEDGER,     
    jtTXN_DATA,      
    jtWAL,           
    jtVALIDATION_t,  
    jtWRITE,         
    jtACCEPT,        
    jtPROPOSAL_t,    
    jtSWEEP,         
    jtNETOP_CLUSTER, 
    jtNETOP_TIMER,   
    jtADMIN,         

    jtPEER          ,
    jtDISK          ,
    jtTXN_PROC      ,
    jtOB_SETUP      ,
    jtPATH_FIND     ,
    jtHO_READ       ,
    jtHO_WRITE      ,
    jtGENERIC       ,   

    jtNS_SYNC_READ  ,
    jtNS_ASYNC_READ ,
    jtNS_WRITE      ,
};

class Job
{
public:
    using clock_type = std::chrono::steady_clock;

    
    Job ();


    Job (JobType type, std::uint64_t index);

    
    using CancelCallback = std::function <bool(void)>;

    Job (JobType type,
         std::string const& name,
         std::uint64_t index,
         LoadMonitor& lm,
         std::function <void (Job&)> const& job,
         CancelCallback cancelCallback);


    JobType getType () const;

    CancelCallback getCancelCallback () const;

    
    clock_type::time_point const& queue_time () const;

    
    bool shouldCancel () const;

    void doJob ();

    void rename (std::string const& n);

    bool operator< (const Job& j) const;
    bool operator> (const Job& j) const;
    bool operator<= (const Job& j) const;
    bool operator>= (const Job& j) const;

private:
    CancelCallback m_cancelCallback;
    JobType                     mType;
    std::uint64_t               mJobIndex;
    std::function <void (Job&)> mJob;
    std::shared_ptr<LoadEvent>  m_loadEvent;
    std::string                 mName;
    clock_type::time_point m_queue_time;
};

}

#endif








