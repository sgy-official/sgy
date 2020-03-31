

#ifndef BEAST_UTILITY_WRAPPEDSINK_H_INCLUDED
#define BEAST_UTILITY_WRAPPEDSINK_H_INCLUDED

#include <ripple/beast/utility/Journal.h>

namespace beast {



class WrappedSink : public beast::Journal::Sink
{
private:
    beast::Journal::Sink& sink_;
    std::string prefix_;

public:
    explicit
    WrappedSink (beast::Journal::Sink& sink, std::string const& prefix = "")
        : Sink (sink)
        , sink_(sink)
        , prefix_(prefix)
    {
    }

    explicit
    WrappedSink (beast::Journal const& journal, std::string const& prefix = "")
        : WrappedSink (journal.sink(), prefix)
    {
    }

    void prefix (std::string const& s)
    {
        prefix_ = s;
    }

    bool
    active (beast::severities::Severity level) const override
    {
        return sink_.active (level);
    }

    bool
    console () const override
    {
        return sink_.console ();
    }

    void console (bool output) override
    {
        sink_.console (output);
    }

    beast::severities::Severity
    threshold() const override
    {
        return sink_.threshold();
    }

    void threshold (beast::severities::Severity thresh) override
    {
        sink_.threshold (thresh);
    }

    void write (beast::severities::Severity level, std::string const& text) override
    {
        using beast::Journal;
        sink_.write (level, prefix_ + text);
    }
};

}

#endif








