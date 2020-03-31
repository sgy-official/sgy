

#include <ripple/app/tx/impl/BookTip.h>
#include <ripple/basics/Log.h>

namespace ripple {

BookTip::BookTip (ApplyView& view, Book const& book)
    : view_ (view)
    , m_valid (false)
    , m_book (getBookBase (book))
    , m_end (getQualityNext (m_book))
{
}

bool
BookTip::step (beast::Journal j)
{
    if (m_valid)
    {
        if (m_entry)
        {
            offerDelete (view_, m_entry, j);
            m_entry = nullptr;
        }
    }

    for(;;)
    {
        auto const first_page =
            view_.succ (m_book, m_end);

        if (! first_page)
            return false;

        unsigned int di = 0;
        std::shared_ptr<SLE> dir;

        if (dirFirst (view_, *first_page, dir, di, m_index, j))
        {
            m_dir = dir->key();
            m_entry = view_.peek(keylet::offer(m_index));
            m_quality = Quality (getQuality (*first_page));
            m_valid = true;

            m_book = *first_page;

            --m_book;

            break;
        }

        m_book = *first_page;
    }

    return true;
}

}
























