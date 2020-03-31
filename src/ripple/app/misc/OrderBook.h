

#ifndef RIPPLE_APP_MISC_ORDERBOOK_H_INCLUDED
#define RIPPLE_APP_MISC_ORDERBOOK_H_INCLUDED

namespace ripple {


class OrderBook
{
public:
    using pointer = std::shared_ptr <OrderBook>;
    using ref = std::shared_ptr <OrderBook> const&;
    using List = std::vector<pointer>;

    
    OrderBook (uint256 const& base, Book const& book)
            : mBookBase(base), mBook(book)
    {
    }

    uint256 const& getBookBase () const
    {
        return mBookBase;
    }

    Book const& book() const
    {
        return mBook;
    }

    Currency const& getCurrencyIn () const
    {
        return mBook.in.currency;
    }

    Currency const& getCurrencyOut () const
    {
        return mBook.out.currency;
    }

    AccountID const& getIssuerIn () const
    {
        return mBook.in.account;
    }

    AccountID const& getIssuerOut () const
    {
        return mBook.out.account;
    }

private:
    uint256 const mBookBase;
    Book const mBook;
};

} 

#endif








