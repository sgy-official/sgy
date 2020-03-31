

#ifndef TEST_UNIT_TEST_SUITE_JOURNAL_H
#define TEST_UNIT_TEST_SUITE_JOURNAL_H

#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/Journal.h>

namespace ripple {
namespace test {

class SuiteJournalSink : public beast::Journal::Sink
{
    std::string partition_;
    beast::unit_test::suite& suite_;

public:
    SuiteJournalSink(std::string const& partition,
            beast::severities::Severity threshold,
            beast::unit_test::suite& suite)
        : Sink (threshold, false)
        , partition_(partition + " ")
        , suite_ (suite)
    {
    }

    inline bool active(beast::severities::Severity level) const override
    {
        return true;
    }

    void
    write(beast::severities::Severity level, std::string const& text) override;
};

inline void
SuiteJournalSink::write (
    beast::severities::Severity level, std::string const& text)
{
    using namespace beast::severities;

    char const* const s = [level]()
    {
        switch(level)
        {
        case kTrace:    return "TRC:";
        case kDebug:    return "DBG:";
        case kInfo:     return "INF:";
        case kWarning:  return "WRN:";
        case kError:    return "ERR:";
        default:        break;
        case kFatal:    break;
        }
        return "FTL:";
    }();

    if (level >= threshold())
        suite_.log << s << partition_ << text << std::endl;
}

class SuiteJournal
{
    SuiteJournalSink sink_;
    beast::Journal journal_;

public:
    SuiteJournal(std::string const& partition,
            beast::unit_test::suite& suite,
            beast::severities::Severity threshold = beast::severities::kFatal)
        : sink_ (partition, threshold, suite)
        , journal_ (sink_)
    {
    }
    operator beast::Journal&() { return journal_; }
};

} 
} 

#endif








