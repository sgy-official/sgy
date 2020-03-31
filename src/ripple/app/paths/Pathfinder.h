

#ifndef RIPPLE_APP_PATHS_PATHFINDER_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHFINDER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/core/LoadEvent.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STPathSet.h>

namespace ripple {


class Pathfinder
{
public:
    
    Pathfinder (
        std::shared_ptr<RippleLineCache> const& cache,
        AccountID const& srcAccount,
        AccountID const& dstAccount,
        Currency const& uSrcCurrency,
        boost::optional<AccountID> const& uSrcIssuer,
        STAmount const& dstAmount,
        boost::optional<STAmount> const& srcAmount,
        Application& app);
    Pathfinder (Pathfinder const&) = delete;
    Pathfinder& operator= (Pathfinder const&) = delete;
    ~Pathfinder() = default;

    static void initPathTable ();

    bool findPaths (int searchLevel);

    
    void computePathRanks (int maxPaths);

    
    STPathSet
    getBestPaths (
        int maxPaths,
        STPath& fullLiquidityPath,
        STPathSet const& extraPaths,
        AccountID const& srcIssuer);

    enum NodeType
    {
        nt_SOURCE,     
        nt_ACCOUNTS,   
        nt_BOOKS,      
        nt_XRP_BOOK,   
        nt_DEST_BOOK,  
        nt_DESTINATION 
    };

    using PathType = std::vector <NodeType>;

    enum PaymentType
    {
        pt_XRP_to_XRP,
        pt_XRP_to_nonXRP,
        pt_nonXRP_to_XRP,
        pt_nonXRP_to_same,   
        pt_nonXRP_to_nonXRP  
    };

    struct PathRank
    {
        std::uint64_t quality;
        std::uint64_t length;
        STAmount liquidity;
        int index;
    };

private:
    


    STPathSet& addPathsForType (PathType const& type);

    bool issueMatchesOrigin (Issue const&);

    int getPathsOut (
        Currency const& currency,
        AccountID const& account,
        bool isDestCurrency,
        AccountID const& dest);

    void addLink (
        STPath const& currentPath,
        STPathSet& incompletePaths,
        int addFlags);

    void addLinks (
        STPathSet const& currentPaths,
        STPathSet& incompletePaths,
        int addFlags);

    TER getPathLiquidity (
        STPath const& path,            
        STAmount const& minDstAmount,  
        STAmount& amountOut,           
        uint64_t& qualityOut) const;   

    bool isNoRippleOut (STPath const& currentPath);

    bool isNoRipple (
        AccountID const& fromAccount,
        AccountID const& toAccount,
        Currency const& currency);

    void rankPaths (
        int maxPaths,
        STPathSet const& paths,
        std::vector <PathRank>& rankedPaths);

    AccountID mSrcAccount;
    AccountID mDstAccount;
    AccountID mEffectiveDst; 
    STAmount mDstAmount;
    Currency mSrcCurrency;
    boost::optional<AccountID> mSrcIssuer;
    STAmount mSrcAmount;
    
    STAmount mRemainingAmount;
    bool convert_all_;

    std::shared_ptr <ReadView const> mLedger;
    std::unique_ptr<LoadEvent> m_loadEvent;
    std::shared_ptr<RippleLineCache> mRLCache;

    STPathElement mSource;
    STPathSet mCompletePaths;
    std::vector<PathRank> mPathRanks;
    std::map<PathType, STPathSet> mPaths;

    hash_map<Issue, int> mPathsOutCountMap;

    Application& app_;
    beast::Journal j_;

    static std::uint32_t const afADD_ACCOUNTS = 0x001;

    static std::uint32_t const afADD_BOOKS = 0x002;

    static std::uint32_t const afOB_XRP = 0x010;

    static std::uint32_t const afOB_LAST = 0x040;

    static std::uint32_t const afAC_LAST = 0x080;
};

} 

#endif








