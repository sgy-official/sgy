

#include <ripple/basics/random.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/nodestore/Database.h>

namespace ripple {

void
SHAMap::visitLeaves(std::function<void (
    std::shared_ptr<SHAMapItem const> const& item)> const& leafFunction) const
{
    visitNodes(
        [&leafFunction](SHAMapAbstractNode& node)
        {
            if (! node.isInner())
                leafFunction(static_cast<SHAMapTreeNode&>(node).peekItem());
            return true;
        });
}

void
SHAMap::visitNodes(std::function<bool (
    SHAMapAbstractNode&)> const& function) const
{
    assert (root_->isValid ());

    if (! root_)
        return;

    function (*root_);

    if (! root_->isInner ())
        return;

    using StackEntry = std::pair <int, std::shared_ptr<SHAMapInnerNode>>;
    std::stack <StackEntry, std::vector <StackEntry>> stack;

    auto node = std::static_pointer_cast<SHAMapInnerNode>(root_);
    int pos = 0;

    while (1)
    {
        while (pos < 16)
        {
            uint256 childHash;
            if (! node->isEmptyBranch (pos))
            {
                std::shared_ptr<SHAMapAbstractNode> child = descendNoStore (node, pos);
                if (! function (*child))
                    return;

                if (child->isLeaf ())
                    ++pos;
                else
                {
                    while ((pos != 15) && (node->isEmptyBranch (pos + 1)))
                           ++pos;

                    if (pos != 15)
                    {
                        stack.push (std::make_pair(pos + 1, std::move (node)));
                    }

                    node = std::static_pointer_cast<SHAMapInnerNode>(child);
                    pos = 0;
                }
            }
            else
            {
                ++pos; 
            }
        }

        if (stack.empty ())
            break;

        std::tie(pos, node) = stack.top ();
        stack.pop ();
    }
}

void
SHAMap::visitDifferences(SHAMap const* have,
    std::function<bool (SHAMapAbstractNode&)> function) const
{
    assert (root_->isValid ());

    if (! root_)
        return;

    if (root_->getNodeHash ().isZero ())
        return;

    if (have && (root_->getNodeHash () == have->root_->getNodeHash ()))
        return;

    if (root_->isLeaf ())
    {
        auto leaf = std::static_pointer_cast<SHAMapTreeNode>(root_);
        if (! have || ! have->hasLeafNode(leaf->peekItem()->key(), leaf->getNodeHash()))
            function (*root_);
        return;
    }
    using StackEntry = std::pair <SHAMapInnerNode*, SHAMapNodeID>;
    std::stack <StackEntry, std::vector<StackEntry>> stack;

    stack.push ({static_cast<SHAMapInnerNode*>(root_.get()), SHAMapNodeID{}});

    while (! stack.empty())
    {
        SHAMapInnerNode* node;
        SHAMapNodeID nodeID;
        std::tie (node, nodeID) = stack.top ();
        stack.pop ();

        if (! function (*node))
            return;

        for (int i = 0; i < 16; ++i)
        {
            if (! node->isEmptyBranch (i))
            {
                auto const& childHash = node->getChildHash (i);
                SHAMapNodeID childID = nodeID.getChildNodeID (i);
                auto next = descendThrow(node, i);

                if (next->isInner ())
                {
                    if (! have || ! have->hasInnerNode(childID, childHash))
                        stack.push ({static_cast<SHAMapInnerNode*>(next), childID});
                }
                else if (! have || ! have->hasLeafNode(
                         static_cast<SHAMapTreeNode*>(next)->peekItem()->key(),
                         childHash))
                {
                    if (! function (*next))
                        return;
                }
            }
        }
    }
}

void SHAMap::gmn_ProcessNodes (MissingNodes& mn, MissingNodes::StackEntry& se)
{
    SHAMapInnerNode*& node = std::get<0>(se);
    SHAMapNodeID&     nodeID = std::get<1>(se);
    int&              firstChild = std::get<2>(se);
    int&              currentChild = std::get<3>(se);
    bool&             fullBelow = std::get<4>(se);

    while (currentChild < 16)
    {
        int branch = (firstChild + currentChild++) % 16;
        if (node->isEmptyBranch (branch))
            continue;

        auto const& childHash = node->getChildHash (branch);

        if (mn.missingHashes_.count (childHash) != 0)
        {
            fullBelow = false;
        }
        else if (! backed_ || ! f_.fullbelow().touch_if_exists (childHash.as_uint256()))
        {
            SHAMapNodeID childID = nodeID.getChildNodeID (branch);
            bool pending = false;
            auto d = descendAsync (node, branch, mn.filter_, pending);

            if (!d)
            {
                fullBelow = false; 

                if (! pending)
                { 
                    mn.missingHashes_.insert (childHash);
                    mn.missingNodes_.emplace_back (
                        childID, childHash.as_uint256());

                    if (--mn.max_ <= 0)
                        return;
                }
                else
                    mn.deferredReads_.emplace_back (node, nodeID, branch);
            }
            else if (d->isInner() &&
                 ! static_cast<SHAMapInnerNode*>(d)->isFullBelow(mn.generation_))
            {
                mn.stack_.push (se);

                node = static_cast<SHAMapInnerNode*>(d);
                if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(node))
                    nodeID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
                else
                    nodeID = childID;
                firstChild = rand_int(255);
                currentChild = 0;
                fullBelow = true;
            }
        }
    }


    if (fullBelow)
    { 
        node->setFullBelowGen (mn.generation_);
        if (backed_)
            f_.fullbelow().insert (node->getNodeHash ().as_uint256());
    }

    node = nullptr;
}

void SHAMap::gmn_ProcessDeferredReads (MissingNodes& mn)
{
    auto const before = std::chrono::steady_clock::now();
    f_.db().waitReads();
    auto const after = std::chrono::steady_clock::now();

    auto const elapsed = std::chrono::duration_cast
        <std::chrono::milliseconds> (after - before);
    auto const count = mn.deferredReads_.size ();

    int hits = 0;
    for (auto const& deferredNode : mn.deferredReads_)
    {
        auto parent = std::get<0>(deferredNode);
        auto const& parentID = std::get<1>(deferredNode);
        auto branch = std::get<2>(deferredNode);
        auto const& nodeHash = parent->getChildHash (branch);

        auto nodePtr = fetchNodeNT(nodeHash, mn.filter_);
        if (nodePtr)
        { 
            ++hits;
            if (backed_)
                canonicalize (nodeHash, nodePtr);
            nodePtr = parent->canonicalizeChild (branch, std::move(nodePtr));

            mn.resumes_[parent] = parentID;
        }
        else if ((mn.max_ > 0) &&
            (mn.missingHashes_.insert (nodeHash).second))
        {
            mn.missingNodes_.emplace_back (
                parentID.getChildNodeID (branch),
                nodeHash.as_uint256());

            --mn.max_;
        }
    }
    mn.deferredReads_.clear();

    auto const process_time = std::chrono::duration_cast
        <std::chrono::milliseconds> (std::chrono::steady_clock::now() - after);

    using namespace std::chrono_literals;
    if ((count > 50) || (elapsed > 50ms))
    {
        JLOG(journal_.debug()) << "getMissingNodes reads " <<
            count << " nodes (" << hits << " hits) in "
            << elapsed.count() << " + " << process_time.count()  << " ms";
    }
}


std::vector<std::pair<SHAMapNodeID, uint256>>
SHAMap::getMissingNodes(int max, SHAMapSyncFilter* filter)
{
    assert (root_->isValid ());
    assert (root_->getNodeHash().isNonZero ());
    assert (max > 0);

    MissingNodes mn (max, filter,
        f_.db().getDesiredAsyncReadCount(ledgerSeq_),
        f_.fullbelow().getGeneration());

    if (! root_->isInner () ||
            std::static_pointer_cast<SHAMapInnerNode>(root_)->
                isFullBelow (mn.generation_))
    {
        clearSynching ();
        return std::move (mn.missingNodes_);
    }

    MissingNodes::StackEntry pos {
        static_cast<SHAMapInnerNode*>(root_.get()), SHAMapNodeID(),
        rand_int(255), 0, true };
    auto& node = std::get<0>(pos);
    auto& nextChild = std::get<3>(pos);
    auto& fullBelow = std::get<4>(pos);

    do
    {

        while ((node != nullptr) &&
            (mn.deferredReads_.size() <= mn.maxDefer_))
        {
            gmn_ProcessNodes (mn, pos);

            if (mn.max_ <= 0)
                return std::move(mn.missingNodes_);

            if ((node == nullptr) && ! mn.stack_.empty ())
            {
                bool was = fullBelow; 

                pos = mn.stack_.top();
                mn.stack_.pop ();
                if (nextChild == 0)
                {
                    fullBelow = true;
                }
                else
                {
                    fullBelow = fullBelow && was; 
                }
                assert (node);
            }
        }


        if (! mn.deferredReads_.empty ())
            gmn_ProcessDeferredReads(mn);

        if (mn.max_ <= 0)
            return std::move(mn.missingNodes_);

        if (node == nullptr)
        { 

            if (mn.stack_.empty() && ! mn.resumes_.empty())
            {
                for (auto& r : mn.resumes_)
                    if (! r.first->isFullBelow (mn.generation_))
                        mn.stack_.push (std::make_tuple (
                            r.first, r.second, rand_int(255), 0, true));

                mn.resumes_.clear();
            }

            if (! mn.stack_.empty())
            {
                pos = mn.stack_.top();
                mn.stack_.pop();
                assert (node != nullptr);
            }
        }


    } while (node != nullptr);

    if (mn.missingNodes_.empty ())
        clearSynching ();

    return std::move(mn.missingNodes_);
}

std::vector<uint256> SHAMap::getNeededHashes (int max, SHAMapSyncFilter* filter)
{
    auto ret = getMissingNodes(max, filter);

    std::vector<uint256> hashes;
    hashes.reserve (ret.size());

    for (auto const& n : ret)
        hashes.push_back (n.second);

    return hashes;
}

bool SHAMap::getNodeFat (SHAMapNodeID wanted,
    std::vector<SHAMapNodeID>& nodeIDs,
        std::vector<Blob>& rawNodes, bool fatLeaves,
            std::uint32_t depth) const
{

    auto node = root_.get();
    SHAMapNodeID nodeID;

    while (node && node->isInner () && (nodeID.getDepth() < wanted.getDepth()))
    {
        int branch = nodeID.selectBranch (wanted.getNodeID());
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
            return false;

        node = descendThrow(inner, branch);
        if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(node))
            nodeID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
        else
            nodeID = nodeID.getChildNodeID (branch);
    }

    if (node == nullptr ||
           (dynamic_cast<SHAMapInnerNodeV2*>(node) != nullptr &&
                !wanted.has_common_prefix(nodeID)) ||
           (dynamic_cast<SHAMapInnerNodeV2*>(node) == nullptr && wanted != nodeID))
    {
        JLOG(journal_.warn())
            << "peer requested node that is not in the map:\n"
            << wanted << " but found\n" << nodeID;
        return false;
    }

    if (node->isInner() && static_cast<SHAMapInnerNode*>(node)->isEmpty())
    {
        JLOG(journal_.warn()) << "peer requests empty node";
        return false;
    }

    std::stack<std::tuple <SHAMapAbstractNode*, SHAMapNodeID, int>> stack;
    stack.emplace (node, nodeID, depth);

    while (! stack.empty ())
    {
        std::tie (node, nodeID, depth) = stack.top ();
        stack.pop ();

        Serializer s;
        node->addRaw (s, snfWIRE);
        nodeIDs.push_back (nodeID);
        rawNodes.push_back (std::move (s.peekData()));

        if (node->isInner())
        {
            auto inner = static_cast<SHAMapInnerNode*>(node);
            int bc = inner->getBranchCount();
            if ((depth > 0) || (bc == 1))
            {
                for (int i = 0; i < 16; ++i)
                {
                    if (! inner->isEmptyBranch (i))
                    {
                        auto childNode = descendThrow (inner, i);
                        SHAMapNodeID childID;
                        if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(childNode))
                            childID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
                        else
                            childID = nodeID.getChildNodeID (i);

                        if (childNode->isInner () &&
                            ((depth > 1) || (bc == 1)))
                        {
                            stack.emplace (childNode, childID,
                                (bc > 1) ? (depth - 1) : depth);
                        }
                        else if (childNode->isInner() || fatLeaves)
                        {
                            Serializer ns;
                            childNode->addRaw (ns, snfWIRE);
                            nodeIDs.push_back (childID);
                            rawNodes.push_back (std::move (ns.peekData ()));
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool SHAMap::getRootNode (Serializer& s, SHANodeFormat format) const
{
    root_->addRaw (s, format);
    return true;
}

SHAMapAddNode SHAMap::addRootNode (SHAMapHash const& hash, Slice const& rootNode, SHANodeFormat format,
                                   SHAMapSyncFilter* filter)
{
    if (root_->getNodeHash ().isNonZero ())
    {
        JLOG(journal_.trace()) << "got root node, already have one";
        assert (root_->getNodeHash () == hash);
        return SHAMapAddNode::duplicate ();
    }

    assert (seq_ >= 1);
    auto node = SHAMapAbstractNode::make(
        rootNode, 0, format, SHAMapHash{}, false, f_.journal ());
    if (!node || !node->isValid() || node->getNodeHash () != hash)
        return SHAMapAddNode::invalid ();

    if (backed_)
        canonicalize (hash, node);

    root_ = node;

    if (root_->isLeaf())
        clearSynching ();

    if (filter)
    {
        Serializer s;
        root_->addRaw (s, snfPREFIX);
        filter->gotNode (false, root_->getNodeHash (), ledgerSeq_,
                         std::move(s.modData ()), root_->getType ());
    }

    return SHAMapAddNode::useful ();
}

SHAMapAddNode
SHAMap::addKnownNode (const SHAMapNodeID& node, Slice const& rawNode,
                      SHAMapSyncFilter* filter)
{
    assert (!node.isRoot ());

    if (!isSynching ())
    {
        JLOG(journal_.trace()) << "AddKnownNode while not synching";
        return SHAMapAddNode::duplicate ();
    }

    std::uint32_t generation = f_.fullbelow().getGeneration();
    auto newNode = SHAMapAbstractNode::make(rawNode, 0, snfWIRE,
                      SHAMapHash{}, false, f_.journal(), node);
    SHAMapNodeID iNodeID;
    auto iNode = root_.get();

    while (iNode->isInner () &&
           !static_cast<SHAMapInnerNode*>(iNode)->isFullBelow(generation) &&
           (iNodeID.getDepth () < node.getDepth ()))
    {
        int branch = iNodeID.selectBranch (node.getNodeID ());
        assert (branch >= 0);
        auto inner = static_cast<SHAMapInnerNode*>(iNode);
        if (inner->isEmptyBranch (branch))
        {
            JLOG(journal_.warn()) << "Add known node for empty branch" << node;
            return SHAMapAddNode::invalid ();
        }

        auto childHash = inner->getChildHash (branch);
        if (f_.fullbelow().touch_if_exists (childHash.as_uint256()))
            return SHAMapAddNode::duplicate ();

        auto prevNode = inner;
        std::tie(iNode, iNodeID) = descend(inner, iNodeID, branch, filter);

        if (iNode == nullptr)
        {
            if (!newNode || !newNode->isValid() || childHash != newNode->getNodeHash ())
            {
                JLOG(journal_.warn()) << "Corrupt node received";
                return SHAMapAddNode::invalid ();
            }

            if (!newNode->isInBounds (iNodeID))
            {
                state_ = SHAMapState::Invalid;
                return SHAMapAddNode::useful ();
            }

            if (newNode && isInconsistentNode(newNode))
            {
                state_ = SHAMapState::Invalid;
                return SHAMapAddNode::useful();
            }

            if ((std::dynamic_pointer_cast<SHAMapInnerNodeV2>(newNode) && !iNodeID.has_common_prefix(node)) ||
               (!std::dynamic_pointer_cast<SHAMapInnerNodeV2>(newNode) && iNodeID != node))
            {
                JLOG(journal_.warn()) << "unable to hook node " << node;
                JLOG(journal_.info()) << " stuck at " << iNodeID;
                JLOG(journal_.info()) <<
                    "got depth=" << node.getDepth () <<
                        ", walked to= " << iNodeID.getDepth ();
                return SHAMapAddNode::useful ();
            }

            if (backed_)
                canonicalize (childHash, newNode);

            newNode = prevNode->canonicalizeChild (branch, std::move(newNode));

            if (filter)
            {
                Serializer s;
                newNode->addRaw (s, snfPREFIX);
                filter->gotNode (false, childHash, ledgerSeq_,
                                 std::move(s.modData ()), newNode->getType ());
            }

            return SHAMapAddNode::useful ();
        }
    }

    JLOG(journal_.trace()) << "got node, already had it (late)";
    return SHAMapAddNode::duplicate ();
}

bool SHAMap::deepCompare (SHAMap& other) const
{
    std::stack <std::pair <SHAMapAbstractNode*, SHAMapAbstractNode*> > stack;

    stack.push ({root_.get(), other.root_.get()});

    while (!stack.empty ())
    {
        SHAMapAbstractNode* node;
        SHAMapAbstractNode* otherNode;
        std::tie(node, otherNode) = stack.top ();
        stack.pop ();

        if (!node || !otherNode)
        {
            JLOG(journal_.info()) << "unable to fetch node";
            return false;
        }
        else if (otherNode->getNodeHash () != node->getNodeHash ())
        {
            JLOG(journal_.warn()) << "node hash mismatch";
            return false;
        }

        if (node->isLeaf ())
        {
            if (!otherNode->isLeaf ())
                 return false;
            auto& nodePeek = static_cast<SHAMapTreeNode*>(node)->peekItem();
            auto& otherNodePeek = static_cast<SHAMapTreeNode*>(otherNode)->peekItem();
            if (nodePeek->key() != otherNodePeek->key())
                return false;
            if (nodePeek->peekData() != otherNodePeek->peekData())
                return false;
        }
        else if (node->isInner ())
        {
            if (!otherNode->isInner ())
                return false;
            auto node_inner = static_cast<SHAMapInnerNode*>(node);
            auto other_inner = static_cast<SHAMapInnerNode*>(otherNode);
            for (int i = 0; i < 16; ++i)
            {
                if (node_inner->isEmptyBranch (i))
                {
                    if (!other_inner->isEmptyBranch (i))
                        return false;
                }
                else
                {
                    if (other_inner->isEmptyBranch (i))
                       return false;

                    auto next = descend(node_inner, i);
                    auto otherNext = other.descend(other_inner, i);
                    if (!next || !otherNext)
                    {
                        JLOG(journal_.warn()) << "unable to fetch inner node";
                        return false;
                    }
                    stack.push ({next, otherNext});
                }
            }
        }
    }

    return true;
}


bool
SHAMap::hasInnerNode (SHAMapNodeID const& targetNodeID,
                      SHAMapHash const& targetNodeHash) const
{
    auto node = root_.get();
    SHAMapNodeID nodeID;

    while (node->isInner () && (nodeID.getDepth () < targetNodeID.getDepth ()))
    {
        int branch = nodeID.selectBranch (targetNodeID.getNodeID ());
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
            return false;

        node = descendThrow (inner, branch);
        nodeID = nodeID.getChildNodeID (branch);
    }

    return (node->isInner()) && (node->getNodeHash() == targetNodeHash);
}


bool
SHAMap::hasLeafNode (uint256 const& tag, SHAMapHash const& targetNodeHash) const
{
    auto node = root_.get();
    SHAMapNodeID nodeID;

    if (!node->isInner()) 
        return node->getNodeHash() == targetNodeHash;

    do
    {
        int branch = nodeID.selectBranch (tag);
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
            return false;   

        if (inner->getChildHash (branch) == targetNodeHash) 
            return true;

        node = descendThrow(inner, branch);
        nodeID = nodeID.getChildNodeID (branch);
    }
    while (node->isInner());

    return false; 
}


void SHAMap::getFetchPack (SHAMap const* have, bool includeLeaves, int max,
                           std::function<void (SHAMapHash const&, const Blob&)> func) const
{
    if (have != nullptr && have->is_v2() != is_v2())
    {
        JLOG(journal_.info()) << "Can not get fetch pack when versions are different.";
        return;
    }
    visitDifferences (have,
        [includeLeaves, &max, &func] (SHAMapAbstractNode& smn) -> bool
        {
            if (includeLeaves || smn.isInner ())
            {
                Serializer s;
                smn.addRaw (s, snfPREFIX);
                func (smn.getNodeHash(), s.peekData());

                if (--max <= 0)
                    return false;
            }
            return true;
        });
}

} 
























