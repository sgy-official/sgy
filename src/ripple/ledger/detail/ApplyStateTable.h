

#ifndef RIPPLE_LEDGER_APPLYSTATETABLE_H_INCLUDED
#define RIPPLE_LEDGER_APPLYSTATETABLE_H_INCLUDED

#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/TxMeta.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>

namespace ripple {
namespace detail {

class ApplyStateTable
{
public:
    using key_type = ReadView::key_type;

private:
    enum class Action
    {
        cache,
        erase,
        insert,
        modify,
    };

    using items_t = std::map<key_type,
        std::pair<Action, std::shared_ptr<SLE>>>;

    items_t items_;
    XRPAmount dropsDestroyed_ = 0;

public:
    ApplyStateTable() = default;
    ApplyStateTable (ApplyStateTable&&) = default;

    ApplyStateTable (ApplyStateTable const&) = delete;
    ApplyStateTable& operator= (ApplyStateTable&&) = delete;
    ApplyStateTable& operator= (ApplyStateTable const&) = delete;

    void
    apply (RawView& to) const;

    void
    apply (OpenView& to, STTx const& tx,
        TER ter, boost::optional<
            STAmount> const& deliver,
                beast::Journal j);

    bool
    exists (ReadView const& base,
        Keylet const& k) const;

    boost::optional<key_type>
    succ (ReadView const& base,
        key_type const& key, boost::optional<
            key_type> const& last) const;

    std::shared_ptr<SLE const>
    read (ReadView const& base,
        Keylet const& k) const;

    std::shared_ptr<SLE>
    peek (ReadView const& base,
        Keylet const& k);

    std::size_t
    size () const;

    void
    visit (ReadView const& base,
        std::function <void (
            uint256 const& key,
            bool isDelete,
            std::shared_ptr <SLE const> const& before,
            std::shared_ptr <SLE const> const& after)> const& func) const;

    void
    erase (ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    rawErase (ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    insert(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    update(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    replace(ReadView const& base,
        std::shared_ptr<SLE> const& sle);

    void
    destroyXRP (XRPAmount const& fee);

    XRPAmount const& dropsDestroyed () const
    {
        return dropsDestroyed_;
    }

private:
    using Mods = hash_map<key_type,
        std::shared_ptr<SLE>>;

    static
    void
    threadItem (TxMeta& meta,
        std::shared_ptr<SLE> const& to);

    std::shared_ptr<SLE>
    getForMod (ReadView const& base,
        key_type const& key, Mods& mods,
            beast::Journal j);

    void
    threadTx (ReadView const& base, TxMeta& meta,
        AccountID const& to, Mods& mods,
            beast::Journal j);

    void
    threadOwners (ReadView const& base,
        TxMeta& meta, std::shared_ptr<
            SLE const> const& sle, Mods& mods,
                beast::Journal j);
};

} 
} 

#endif








