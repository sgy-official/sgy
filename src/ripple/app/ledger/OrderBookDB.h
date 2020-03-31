

#ifndef RIPPLE_APP_LEDGER_ORDERBOOKDB_H_INCLUDED
#define RIPPLE_APP_LEDGER_ORDERBOOKDB_H_INCLUDED

#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/app/ledger/BookListeners.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/OrderBook.h>
#include <mutex>

namespace ripple {

class OrderBookDB
    : public Stoppable
{
public:
    OrderBookDB (Application& app, Stoppable& parent);

    void setup (std::shared_ptr<ReadView const> const& ledger);
    void update (std::shared_ptr<ReadView const> const& ledger);
    void invalidate ();

    void addOrderBook(Book const&);

    
    OrderBook::List getBooksByTakerPays (Issue const&);

    
    int getBookSize(Issue const&);

    bool isBookToXRP (Issue const&);

    BookListeners::pointer getBookListeners (Book const&);
    BookListeners::pointer makeBookListeners (Book const&);

    void processTxn (
        std::shared_ptr<ReadView const> const& ledger,
        const AcceptedLedgerTx& alTx, Json::Value const& jvObj);

    using IssueToOrderBook = hash_map <Issue, OrderBook::List>;

private:
    void rawAddBook(Book const&);

    Application& app_;

    IssueToOrderBook mSourceMap;

    IssueToOrderBook mDestMap;

    hash_set <Issue> mXRPBooks;

    std::recursive_mutex mLock;

    using BookToListenersMap = hash_map <Book, BookListeners::pointer>;

    BookToListenersMap mListeners;

    std::uint32_t mSeq;

    beast::Journal j_;
};

} 

#endif








