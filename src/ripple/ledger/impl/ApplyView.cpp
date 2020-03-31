

#include <ripple/ledger/ApplyView.h>
#include <ripple/basics/contract.h>
#include <ripple/protocol/Protocol.h>
#include <cassert>

namespace ripple {

boost::optional<std::uint64_t>
ApplyView::dirAdd (
    bool preserveOrder,
    Keylet const& directory,
    uint256 const& key,
    std::function<void(std::shared_ptr<SLE> const&)> const& describe)
{
    auto root = peek(directory);

    if (! root)
    {
        root = std::make_shared<SLE>(directory);
        root->setFieldH256 (sfRootIndex, directory.key);
        describe (root);

        STVector256 v;
        v.push_back (key);
        root->setFieldV256 (sfIndexes, v);

        insert (root);
        return std::uint64_t{0};
    }

    std::uint64_t page = root->getFieldU64(sfIndexPrevious);

    auto node = root;

    if (page)
    {
        node = peek (keylet::page(directory, page));
        if (!node)
            LogicError ("Directory chain: root back-pointer broken.");
    }

    auto indexes = node->getFieldV256(sfIndexes);

    if (indexes.size () < dirNodeMaxEntries)
    {
        if (preserveOrder)
        {
            if (std::find(indexes.begin(), indexes.end(), key) != indexes.end())
                LogicError ("dirInsert: double insertion");

            indexes.push_back(key);
        }
        else
        {
            std::sort (indexes.begin(), indexes.end());

            auto pos = std::lower_bound(indexes.begin(), indexes.end(), key);

            if (pos != indexes.end() && key == *pos)
                LogicError ("dirInsert: double insertion");

            indexes.insert (pos, key);
        }

        node->setFieldV256 (sfIndexes, indexes);
        update(node);
        return page;
    }

    if (++page >= dirNodeMaxPages)
        return boost::none;

    node->setFieldU64 (sfIndexNext, page);
    update(node);

    root->setFieldU64 (sfIndexPrevious, page);
    update(root);

    indexes.clear();
    indexes.push_back (key);

    node = std::make_shared<SLE>(keylet::page(directory, page));
    node->setFieldH256 (sfRootIndex, directory.key);
    node->setFieldV256 (sfIndexes, indexes);

    if (page != 1)
        node->setFieldU64 (sfIndexPrevious, page - 1);
    describe (node);
    insert (node);

    return page;
}

bool
ApplyView::dirRemove (
    Keylet const& directory,
    std::uint64_t page,
    uint256 const& key,
    bool keepRoot)
{
    auto node = peek(keylet::page(directory, page));

    if (!node)
        return false;

    std::uint64_t constexpr rootPage = 0;

    {
        auto entries = node->getFieldV256(sfIndexes);

        auto it = std::find(entries.begin(), entries.end(), key);

        if (entries.end () == it)
            return false;

        entries.erase(it);

        node->setFieldV256(sfIndexes, entries);
        update(node);

        if (!entries.empty())
            return true;
    }

    auto prevPage = node->getFieldU64(sfIndexPrevious);
    auto nextPage = node->getFieldU64(sfIndexNext);

    if (page == rootPage)
    {
        if (nextPage == page && prevPage != page)
            LogicError ("Directory chain: fwd link broken");

        if (prevPage == page && nextPage != page)
            LogicError ("Directory chain: rev link broken");

        if (nextPage == prevPage && nextPage != page)
        {
            auto last = peek(keylet::page(directory, nextPage));
            if (!last)
                LogicError ("Directory chain: fwd link broken.");

            if (last->getFieldV256 (sfIndexes).empty())
            {
                node->setFieldU64 (sfIndexNext, page);
                node->setFieldU64 (sfIndexPrevious, page);
                update(node);

                erase(last);

                nextPage = page;
                prevPage = page;
            }
        }

        if (keepRoot)
            return true;

        if (nextPage == page && prevPage == page)
            erase(node);

        return true;
    }

    if (nextPage == page)
        LogicError ("Directory chain: fwd link broken");

    if (prevPage == page)
        LogicError ("Directory chain: rev link broken");

    auto prev = peek(keylet::page(directory, prevPage));
    if (!prev)
        LogicError ("Directory chain: fwd link broken.");
    prev->setFieldU64(sfIndexNext, nextPage);
    update (prev);

    auto next = peek(keylet::page(directory, nextPage));
    if (!next)
        LogicError ("Directory chain: rev link broken.");
    next->setFieldU64(sfIndexPrevious, prevPage);
    update(next);

    erase(node);

    if (nextPage != rootPage &&
        next->getFieldU64 (sfIndexNext) == rootPage &&
        next->getFieldV256 (sfIndexes).empty())
    {
        erase(next);

        prev->setFieldU64(sfIndexNext, rootPage);
        update (prev);

        auto root = peek(keylet::page(directory, rootPage));
        if (!root)
            LogicError ("Directory chain: root link broken.");
        root->setFieldU64(sfIndexPrevious, prevPage);
        update (root);

        nextPage = rootPage;
    }

    if (!keepRoot && nextPage == rootPage && prevPage == rootPage)
    {
        if (prev->getFieldV256 (sfIndexes).empty())
            erase(prev);
    }

    return true;
}

} 
























