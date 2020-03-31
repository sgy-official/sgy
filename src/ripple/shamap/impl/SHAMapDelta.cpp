

#include <ripple/basics/contract.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {


bool SHAMap::walkBranch (SHAMapAbstractNode* node,
                         std::shared_ptr<SHAMapItem const> const& otherMapItem,
                         bool isFirstMap,Delta& differences, int& maxCount) const
{
    std::stack <SHAMapAbstractNode*, std::vector<SHAMapAbstractNode*>> nodeStack;
    nodeStack.push (node);

    bool emptyBranch = !otherMapItem;

    while (!nodeStack.empty ())
    {
        node = nodeStack.top ();
        nodeStack.pop ();

        if (node->isInner ())
        {
            auto inner = static_cast<SHAMapInnerNode*>(node);
            for (int i = 0; i < 16; ++i)
                if (!inner->isEmptyBranch (i))
                    nodeStack.push ({descendThrow (inner, i)});
        }
        else
        {
            auto item = static_cast<SHAMapTreeNode*>(node)->peekItem();

            if (emptyBranch || (item->key() != otherMapItem->key()))
            {
                if (isFirstMap)
                    differences.insert (std::make_pair (item->key(),
                        DeltaRef (item, std::shared_ptr<SHAMapItem const> ())));
                else
                    differences.insert (std::make_pair (item->key(),
                        DeltaRef (std::shared_ptr<SHAMapItem const> (), item)));

                if (--maxCount <= 0)
                    return false;
            }
            else if (item->peekData () != otherMapItem->peekData ())
            {
                if (isFirstMap)
                    differences.insert (std::make_pair (item->key(),
                                                DeltaRef (item, otherMapItem)));
                else
                    differences.insert (std::make_pair (item->key(),
                                                DeltaRef (otherMapItem, item)));

                if (--maxCount <= 0)
                    return false;

                emptyBranch = true;
            }
            else
            {
                emptyBranch = true;
            }
        }
    }

    if (!emptyBranch)
    {
        if (isFirstMap) 
            differences.insert(std::make_pair(otherMapItem->key(),
                                              DeltaRef(std::shared_ptr<SHAMapItem const>(),
                                                       otherMapItem)));
        else
            differences.insert(std::make_pair(otherMapItem->key(),
                DeltaRef(otherMapItem, std::shared_ptr<SHAMapItem const>())));

        if (--maxCount <= 0)
            return false;
    }

    return true;
}

bool
SHAMap::compare (SHAMap const& otherMap,
                 Delta& differences, int maxCount) const
{

    assert (isValid () && otherMap.isValid ());

    if (getHash () == otherMap.getHash ())
        return true;

    using StackEntry = std::pair <SHAMapAbstractNode*, SHAMapAbstractNode*>;
    std::stack <StackEntry, std::vector<StackEntry>> nodeStack; 

    nodeStack.push ({root_.get(), otherMap.root_.get()});
    while (!nodeStack.empty ())
    {
        SHAMapAbstractNode* ourNode = nodeStack.top().first;
        SHAMapAbstractNode* otherNode = nodeStack.top().second;
        nodeStack.pop ();

        if (!ourNode || !otherNode)
        {
            assert (false);
            Throw<SHAMapMissingNode> (type_, uint256 ());
        }

        if (ourNode->isLeaf () && otherNode->isLeaf ())
        {
            auto ours = static_cast<SHAMapTreeNode*>(ourNode);
            auto other = static_cast<SHAMapTreeNode*>(otherNode);
            if (ours->peekItem()->key() == other->peekItem()->key())
            {
                if (ours->peekItem()->peekData () != other->peekItem()->peekData ())
                {
                    differences.insert (std::make_pair (ours->peekItem()->key(),
                                                 DeltaRef (ours->peekItem (),
                                                 other->peekItem ())));
                    if (--maxCount <= 0)
                        return false;
                }
            }
            else
            {
                differences.insert (std::make_pair(ours->peekItem()->key(),
                                                   DeltaRef(ours->peekItem(),
                                                   std::shared_ptr<SHAMapItem const>())));
                if (--maxCount <= 0)
                    return false;

                differences.insert(std::make_pair(other->peekItem()->key(),
                    DeltaRef(std::shared_ptr<SHAMapItem const>(), other->peekItem ())));
                if (--maxCount <= 0)
                    return false;
            }
        }
        else if (ourNode->isInner () && otherNode->isLeaf ())
        {
            auto ours = static_cast<SHAMapInnerNode*>(ourNode);
            auto other = static_cast<SHAMapTreeNode*>(otherNode);
            if (!walkBranch (ours, other->peekItem (),
                    true, differences, maxCount))
                return false;
        }
        else if (ourNode->isLeaf () && otherNode->isInner ())
        {
            auto ours = static_cast<SHAMapTreeNode*>(ourNode);
            auto other = static_cast<SHAMapInnerNode*>(otherNode);
            if (!otherMap.walkBranch (other, ours->peekItem (),
                                       false, differences, maxCount))
                return false;
        }
        else if (ourNode->isInner () && otherNode->isInner ())
        {
            auto ours = static_cast<SHAMapInnerNode*>(ourNode);
            auto other = static_cast<SHAMapInnerNode*>(otherNode);
            for (int i = 0; i < 16; ++i)
                if (ours->getChildHash (i) != other->getChildHash (i))
                {
                    if (other->isEmptyBranch (i))
                    {
                        SHAMapAbstractNode* iNode = descendThrow (ours, i);
                        if (!walkBranch (iNode,
                                         std::shared_ptr<SHAMapItem const> (), true,
                                         differences, maxCount))
                            return false;
                    }
                    else if (ours->isEmptyBranch (i))
                    {
                        SHAMapAbstractNode* iNode =
                            otherMap.descendThrow(other, i);
                        if (!otherMap.walkBranch (iNode,
                                                   std::shared_ptr<SHAMapItem const>(),
                                                   false, differences, maxCount))
                            return false;
                    }
                    else 
                        nodeStack.push ({descendThrow (ours, i),
                                        otherMap.descendThrow (other, i)});
                }
        }
        else
            assert (false);
    }

    return true;
}

void SHAMap::walkMap (std::vector<SHAMapMissingNode>& missingNodes, int maxMissing) const
{
    if (!root_->isInner ())  
        return;

    using StackEntry = std::shared_ptr<SHAMapInnerNode>;
    std::stack <StackEntry, std::vector <StackEntry>> nodeStack;

    nodeStack.push (std::static_pointer_cast<SHAMapInnerNode>(root_));

    while (!nodeStack.empty ())
    {
        std::shared_ptr<SHAMapInnerNode> node = std::move (nodeStack.top());
        nodeStack.pop ();

        for (int i = 0; i < 16; ++i)
        {
            if (!node->isEmptyBranch (i))
            {
                std::shared_ptr<SHAMapAbstractNode> nextNode = descendNoStore (node, i);

                if (nextNode)
                {
                    if (nextNode->isInner ())
                        nodeStack.push(
                            std::static_pointer_cast<SHAMapInnerNode>(nextNode));
                }
                else
                {
                    missingNodes.emplace_back (type_, node->getChildHash (i));
                    if (--maxMissing <= 0)
                        return;
                }
            }
        }
    }
}

} 
























