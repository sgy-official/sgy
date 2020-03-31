

#ifndef RIPPLE_SHAMAP_SHAMAP_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAP_H_INCLUDED

#include <ripple/shamap/Family.h>
#include <ripple/shamap/FullBelowCache.h>
#include <ripple/shamap/SHAMapAddNode.h>
#include <ripple/shamap/SHAMapItem.h>
#include <ripple/shamap/SHAMapMissingNode.h>
#include <ripple/shamap/SHAMapNodeID.h>
#include <ripple/shamap/SHAMapSyncFilter.h>
#include <ripple/shamap/SHAMapTreeNode.h>
#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/nodestore/Database.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cassert>
#include <stack>
#include <vector>

namespace ripple {

enum class SHAMapState
{
    Modifying = 0,       
    Immutable = 1,       
    Synching  = 2,       
    Floating  = 3,       
    Invalid   = 4,       
};


using MissingNodeHandler = std::function <void (std::uint32_t refNum)>;


class SHAMap
{
private:
    Family&                         f_;
    beast::Journal                  journal_;
    std::uint32_t                   seq_;
    std::uint32_t                   ledgerSeq_ = 0; 
    std::shared_ptr<SHAMapAbstractNode> root_;
    mutable SHAMapState             state_;
    SHAMapType                      type_;
    bool                            backed_ = true; 
    bool                            full_ = false; 

public:
    class version
    {
        int v_;
    public:
        explicit version(int v) : v_(v) {}

        friend bool operator==(version const& x, version const& y)
            {return x.v_ == y.v_;}
        friend bool operator!=(version const& x, version const& y)
            {return !(x == y);}
    };

    using DeltaItem = std::pair<std::shared_ptr<SHAMapItem const>,
                                std::shared_ptr<SHAMapItem const>>;
    using Delta     = std::map<uint256, DeltaItem>;

    ~SHAMap ();
    SHAMap(SHAMap const&) = delete;
    SHAMap& operator=(SHAMap const&) = delete;

    SHAMap (
        SHAMapType t,
        Family& f,
        version v
        );

    SHAMap (
        SHAMapType t,
        uint256 const& hash,
        Family& f,
        version v);

    Family const&
    family() const
    {
        return f_;
    }

    Family&
    family()
    {
        return f_;
    }


    
    class const_iterator;

    const_iterator begin() const;
    const_iterator end() const;


    std::shared_ptr<SHAMap> snapShot (bool isMutable) const;

    
    void setFull ();

    void setLedgerSeq (std::uint32_t lseq);

    bool fetchRoot (SHAMapHash const& hash, SHAMapSyncFilter * filter);

    bool hasItem (uint256 const& id) const;
    bool delItem (uint256 const& id);
    bool addItem (SHAMapItem&& i, bool isTransaction, bool hasMeta);
    SHAMapHash getHash () const;

    bool updateGiveItem (std::shared_ptr<SHAMapItem const> const&,
                         bool isTransaction, bool hasMeta);
    bool addGiveItem (std::shared_ptr<SHAMapItem const> const&,
                      bool isTransaction, bool hasMeta);

    std::shared_ptr<SHAMapItem const> const& peekItem (uint256 const& id) const;
    std::shared_ptr<SHAMapItem const> const&
        peekItem (uint256 const& id, SHAMapHash& hash) const;
    std::shared_ptr<SHAMapItem const> const&
        peekItem (uint256 const& id, SHAMapTreeNode::TNType & type) const;

    const_iterator upper_bound(uint256 const& id) const;

    
    void visitNodes (std::function<bool (
        SHAMapAbstractNode&)> const& function) const;

    
    void visitDifferences(SHAMap const* have,
        std::function<bool (SHAMapAbstractNode&)>) const;

    
    void visitLeaves(std::function<void (
        std::shared_ptr<SHAMapItem const> const&)> const&) const;


    
    std::vector<std::pair<SHAMapNodeID, uint256>>
    getMissingNodes (int maxNodes, SHAMapSyncFilter *filter);

    bool getNodeFat (SHAMapNodeID node,
        std::vector<SHAMapNodeID>& nodeIDs,
            std::vector<Blob>& rawNode,
                bool fatLeaves, std::uint32_t depth) const;

    bool getRootNode (Serializer & s, SHANodeFormat format) const;
    std::vector<uint256> getNeededHashes (int max, SHAMapSyncFilter * filter);
    SHAMapAddNode addRootNode (SHAMapHash const& hash, Slice const& rootNode,
                               SHANodeFormat format, SHAMapSyncFilter * filter);
    SHAMapAddNode addKnownNode (SHAMapNodeID const& nodeID, Slice const& rawNode,
                                SHAMapSyncFilter * filter);


    void setImmutable ();
    bool isSynching () const;
    void setSynching ();
    void clearSynching ();
    bool isValid () const;

    bool compare (SHAMap const& otherMap,
                  Delta& differences, int maxCount) const;

    int flushDirty (NodeObjectType t, std::uint32_t seq);
    void walkMap (std::vector<SHAMapMissingNode>& missingNodes, int maxMissing) const;
    bool deepCompare (SHAMap & other) const;  

    using fetchPackEntry_t = std::pair <uint256, Blob>;

    void getFetchPack (SHAMap const* have, bool includeLeaves, int max,
        std::function<void (SHAMapHash const&, const Blob&)>) const;

    void setUnbacked ();
    bool is_v2() const;
    version get_version() const;
    std::shared_ptr<SHAMap> make_v1() const;
    std::shared_ptr<SHAMap> make_v2() const;
    int unshare ();

    void dump (bool withHashes = false) const;
    void invariants() const;

private:
    using SharedPtrNodeStack =
        std::stack<std::pair<std::shared_ptr<SHAMapAbstractNode>, SHAMapNodeID>>;
    using DeltaRef = std::pair<std::shared_ptr<SHAMapItem const> const&,
                               std::shared_ptr<SHAMapItem const> const&>;

    std::shared_ptr<SHAMapAbstractNode> getCache (SHAMapHash const& hash) const;
    void canonicalize (SHAMapHash const& hash, std::shared_ptr<SHAMapAbstractNode>&) const;

    std::shared_ptr<SHAMapAbstractNode> fetchNodeFromDB (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNodeNT (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNodeNT (
        SHAMapHash const& hash,
        SHAMapSyncFilter *filter) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNode (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> checkFilter(SHAMapHash const& hash,
        SHAMapSyncFilter* filter) const;

    
    void dirtyUp (SharedPtrNodeStack& stack,
                  uint256 const& target, std::shared_ptr<SHAMapAbstractNode> terminal);

    
    SHAMapTreeNode*
        walkTowardsKey(uint256 const& id, SharedPtrNodeStack* stack = nullptr) const;
    
    SHAMapTreeNode*
        findKey(uint256 const& id) const;

    
    template <class Node>
        std::shared_ptr<Node>
        unshareNode(std::shared_ptr<Node>, SHAMapNodeID const& nodeID);

    
    template <class Node>
        std::shared_ptr<Node>
        preFlushNode(std::shared_ptr<Node> node) const;

    
    std::shared_ptr<SHAMapAbstractNode>
        writeNode(NodeObjectType t, std::uint32_t seq,
                  std::shared_ptr<SHAMapAbstractNode> node) const;

    SHAMapTreeNode* firstBelow (std::shared_ptr<SHAMapAbstractNode>,
                                SharedPtrNodeStack& stack, int branch = 0) const;

    SHAMapAbstractNode* descend (SHAMapInnerNode*, int branch) const;
    SHAMapAbstractNode* descendThrow (SHAMapInnerNode*, int branch) const;
    std::shared_ptr<SHAMapAbstractNode> descend (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;
    std::shared_ptr<SHAMapAbstractNode> descendThrow (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;

    SHAMapAbstractNode* descendAsync (SHAMapInnerNode* parent, int branch,
        SHAMapSyncFilter* filter, bool& pending) const;

    std::pair <SHAMapAbstractNode*, SHAMapNodeID>
        descend (SHAMapInnerNode* parent, SHAMapNodeID const& parentID,
        int branch, SHAMapSyncFilter* filter) const;

    std::shared_ptr<SHAMapAbstractNode>
        descendNoStore (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;

    
    std::shared_ptr<SHAMapItem const> const& onlyBelow (SHAMapAbstractNode*) const;

    bool hasInnerNode (SHAMapNodeID const& nodeID, SHAMapHash const& hash) const;
    bool hasLeafNode (uint256 const& tag, SHAMapHash const& hash) const;

    SHAMapTreeNode const* peekFirstItem(SharedPtrNodeStack& stack) const;
    SHAMapTreeNode const* peekNextItem(uint256 const& id, SharedPtrNodeStack& stack) const;
    bool walkBranch (SHAMapAbstractNode* node,
                     std::shared_ptr<SHAMapItem const> const& otherMapItem,
                     bool isFirstMap, Delta & differences, int & maxCount) const;
    int walkSubTree (bool doWrite, NodeObjectType t, std::uint32_t seq);
    bool isInconsistentNode(std::shared_ptr<SHAMapAbstractNode> const& node) const;

    struct MissingNodes
    {
        MissingNodes() = delete;
        MissingNodes(const MissingNodes&) = delete;
        MissingNodes& operator=(const MissingNodes&) = delete;

        int               max_;
        SHAMapSyncFilter* filter_;
        int const         maxDefer_;
        std::uint32_t     generation_;

        std::vector<std::pair<SHAMapNodeID, uint256>> missingNodes_;
        std::set <SHAMapHash>                         missingHashes_;

        using StackEntry = std::tuple<
            SHAMapInnerNode*, 
            SHAMapNodeID,     
            int,              
            int,              
            bool>;            

        std::stack <StackEntry, std::deque<StackEntry>> stack_;

        std::vector <std::tuple <SHAMapInnerNode*, SHAMapNodeID, int>> deferredReads_;

        std::map<SHAMapInnerNode*, SHAMapNodeID> resumes_;

        MissingNodes (
            int max, SHAMapSyncFilter* filter,
            int maxDefer, std::uint32_t generation) :
                max_(max), filter_(filter),
                maxDefer_(maxDefer), generation_(generation)
        {
            missingNodes_.reserve (max);
            deferredReads_.reserve(maxDefer);
        }
    };

    void gmn_ProcessNodes (MissingNodes&, MissingNodes::StackEntry& node);
    void gmn_ProcessDeferredReads (MissingNodes&);
};

inline
void
SHAMap::setFull ()
{
    full_ = true;
}

inline
void
SHAMap::setLedgerSeq (std::uint32_t lseq)
{
    ledgerSeq_ = lseq;
}

inline
void
SHAMap::setImmutable ()
{
    assert (state_ != SHAMapState::Invalid);
    state_ = SHAMapState::Immutable;
}

inline
bool
SHAMap::isSynching () const
{
    return (state_ == SHAMapState::Floating) || (state_ == SHAMapState::Synching);
}

inline
void
SHAMap::setSynching ()
{
    state_ = SHAMapState::Synching;
}

inline
void
SHAMap::clearSynching ()
{
    state_ = SHAMapState::Modifying;
}

inline
bool
SHAMap::isValid () const
{
    return state_ != SHAMapState::Invalid;
}

inline
void
SHAMap::setUnbacked ()
{
    backed_ = false;
}

inline
bool
SHAMap::is_v2() const
{
    assert (root_);
    return std::dynamic_pointer_cast<SHAMapInnerNodeV2>(root_) != nullptr;
}


class SHAMap::const_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = SHAMapItem;
    using reference         = value_type const&;
    using pointer           = value_type const*;

private:
    SharedPtrNodeStack stack_;
    SHAMap const*      map_  = nullptr;
    pointer            item_ = nullptr;

public:
    const_iterator() = default;

    reference operator*()  const;
    pointer   operator->() const;

    const_iterator& operator++();
    const_iterator  operator++(int);

private:
    explicit const_iterator(SHAMap const* map);
    const_iterator(SHAMap const* map, pointer item);
    const_iterator(SHAMap const* map, pointer item, SharedPtrNodeStack&& stack);

    friend bool operator==(const_iterator const& x, const_iterator const& y);
    friend class SHAMap;
};

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map)
    : map_(map)
    , item_(nullptr)
{
    auto temp = map_->peekFirstItem(stack_);
    if (temp)
        item_ = temp->peekItem().get();
}

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map, pointer item)
    : map_(map)
    , item_(item)
{
}

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map, pointer item,
                                       SharedPtrNodeStack&& stack)
    : stack_(std::move(stack))
    , map_(map)
    , item_(item)
{
}

inline
SHAMap::const_iterator::reference
SHAMap::const_iterator::operator*() const
{
    return *item_;
}

inline
SHAMap::const_iterator::pointer
SHAMap::const_iterator::operator->() const
{
    return item_;
}

inline
SHAMap::const_iterator&
SHAMap::const_iterator::operator++()
{
    auto temp = map_->peekNextItem(item_->key(), stack_);
    if (temp)
        item_ = temp->peekItem().get();
    else
        item_ = nullptr;
    return *this;
}

inline
SHAMap::const_iterator
SHAMap::const_iterator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

inline
bool
operator==(SHAMap::const_iterator const& x, SHAMap::const_iterator const& y)
{
    assert(x.map_ == y.map_);
    return x.item_ == y.item_;
}

inline
bool
operator!=(SHAMap::const_iterator const& x, SHAMap::const_iterator const& y)
{
    return !(x == y);
}

inline
SHAMap::const_iterator
SHAMap::begin() const
{
    return const_iterator(this);
}

inline
SHAMap::const_iterator
SHAMap::end() const
{
    return const_iterator(this, nullptr);
}

}

#endif








