

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerReplay.h>

namespace ripple {

LedgerReplay::LedgerReplay(
    std::shared_ptr<Ledger const> parent,
    std::shared_ptr<Ledger const> replay)
    : parent_{std::move(parent)}, replay_{std::move(replay)}
{
    for (auto const& item : replay_->txMap())
    {
        auto const txPair = replay_->txRead(item.key());
        auto const txIndex = (*txPair.second)[sfTransactionIndex];
        orderedTxns_.emplace(txIndex, std::move(txPair.first));
    }
}

}  
























