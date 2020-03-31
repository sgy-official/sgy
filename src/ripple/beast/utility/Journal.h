

#ifndef BEAST_UTILITY_JOURNAL_H_INCLUDED
#define BEAST_UTILITY_JOURNAL_H_INCLUDED

#include <cassert>
#include <sstream>

namespace beast {


namespace severities
{
    
    enum Severity
    {
        kAll = 0,

        kTrace = kAll,
        kDebug,
        kInfo,
        kWarning,
        kError,
        kFatal,

        kDisabled,
        kNone = kDisabled
    };
}


class Journal
{
public:
    class Sink;

private:
    using Severity = severities::Severity;

    Sink* m_sink;

public:

    
    class Sink
    {
    protected:
        Sink () = delete;
        explicit Sink(Sink const& sink) = default;
        Sink (Severity thresh, bool console);
        Sink& operator= (Sink const& lhs) = delete;

    public:
        virtual ~Sink () = 0;

        
        virtual bool active (Severity level) const;

        
        virtual bool console () const;

        
        virtual void console (bool output);

        
        virtual Severity threshold() const;

        
        virtual void threshold (Severity thresh);

        
        virtual void write (Severity level, std::string const& text) = 0;

    private:
        Severity thresh_;
        bool m_console;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Sink>::value == false, "");
static_assert(std::is_copy_constructible<Sink>::value == false, "");
static_assert(std::is_move_constructible<Sink>::value == false, "");
static_assert(std::is_copy_assignable<Sink>::value == false, "");
static_assert(std::is_move_assignable<Sink>::value == false, "");
static_assert(std::is_nothrow_destructible<Sink>::value == true, "");
#endif

    
    static Sink& getNullSink ();


    class Stream;

private:
    
    class ScopedStream
    {
    public:
        ScopedStream (ScopedStream const& other)
            : ScopedStream (other.m_sink, other.m_level)
        { }

        ScopedStream (Sink& sink, Severity level);

        template <typename T>
        ScopedStream (Stream const& stream, T const& t);

        ScopedStream (
            Stream const& stream, std::ostream& manip (std::ostream&));

        ScopedStream& operator= (ScopedStream const&) = delete;

        ~ScopedStream ();

        std::ostringstream& ostream () const
        {
            return m_ostream;
        }

        std::ostream& operator<< (
            std::ostream& manip (std::ostream&)) const;

        template <typename T>
        std::ostream& operator<< (T const& t) const;

    private:
        Sink& m_sink;
        Severity const m_level;
        std::ostringstream mutable m_ostream;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<ScopedStream>::value == false, "");
static_assert(std::is_copy_constructible<ScopedStream>::value == true, "");
static_assert(std::is_move_constructible<ScopedStream>::value == true, "");
static_assert(std::is_copy_assignable<ScopedStream>::value == false, "");
static_assert(std::is_move_assignable<ScopedStream>::value == false, "");
static_assert(std::is_nothrow_destructible<ScopedStream>::value == true, "");
#endif

public:
    
    class Stream
    {
    public:
        
        explicit Stream ()
            : m_sink (getNullSink())
            , m_level (severities::kDisabled)
        { }

        
        Stream (Sink& sink, Severity level)
            : m_sink (sink)
            , m_level (level)
        {
            assert (m_level < severities::kDisabled);
        }

        
        Stream (Stream const& other)
            : Stream (other.m_sink, other.m_level)
        { }

        Stream& operator= (Stream const& other) = delete;

        
        Sink& sink() const
        {
            return m_sink;
        }

        
        Severity level() const
        {
            return m_level;
        }

        
        
        bool active() const
        {
            return m_sink.active (m_level);
        }

        explicit
        operator bool() const
        {
            return active();
        }
        

        
        
        ScopedStream operator<< (std::ostream& manip (std::ostream&)) const;

        template <typename T>
        ScopedStream operator<< (T const& t) const;
        

    private:
        Sink& m_sink;
        Severity m_level;
    };

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Stream>::value == true, "");
static_assert(std::is_copy_constructible<Stream>::value == true, "");
static_assert(std::is_move_constructible<Stream>::value == true, "");
static_assert(std::is_copy_assignable<Stream>::value == false, "");
static_assert(std::is_move_assignable<Stream>::value == false, "");
static_assert(std::is_nothrow_destructible<Stream>::value == true, "");
#endif


    
    Journal () = delete;

    
    explicit Journal (Sink& sink)
    : m_sink (&sink)
    { }

    
    Sink& sink() const
    {
        return *m_sink;
    }

    
    Stream stream (Severity level) const
    {
        return Stream (*m_sink, level);
    }

    
    bool active (Severity level) const
    {
        return m_sink->active (level);
    }

    
    
    Stream trace() const
    {
        return { *m_sink, severities::kTrace };
    }

    Stream debug() const
    {
        return { *m_sink, severities::kDebug };
    }

    Stream info() const
    {
        return { *m_sink, severities::kInfo };
    }

    Stream warn() const
    {
        return { *m_sink, severities::kWarning };
    }

    Stream error() const
    {
        return { *m_sink, severities::kError };
    }

    Stream fatal() const
    {
        return { *m_sink, severities::kFatal };
    }
    
};

#ifndef __INTELLISENSE__
static_assert(std::is_default_constructible<Journal>::value == false, "");
static_assert(std::is_copy_constructible<Journal>::value == true, "");
static_assert(std::is_move_constructible<Journal>::value == true, "");
static_assert(std::is_copy_assignable<Journal>::value == true, "");
static_assert(std::is_move_assignable<Journal>::value == true, "");
static_assert(std::is_nothrow_destructible<Journal>::value == true, "");
#endif


template <typename T>
Journal::ScopedStream::ScopedStream (Journal::Stream const& stream, T const& t)
   : ScopedStream (stream.sink(), stream.level())
{
    m_ostream << t;
}

template <typename T>
std::ostream&
Journal::ScopedStream::operator<< (T const& t) const
{
    m_ostream << t;
    return m_ostream;
}


template <typename T>
Journal::ScopedStream
Journal::Stream::operator<< (T const& t) const
{
    return ScopedStream (*this, t);
}

namespace detail {

template<class CharT, class Traits = std::char_traits<CharT>>
class logstream_buf
    : public std::basic_stringbuf<CharT, Traits>
{
    beast::Journal::Stream strm_;

    template<class T>
    void write(T const*) = delete;

    void write(char const* s)
    {
        if(strm_)
            strm_ << s;
    }

    void write(wchar_t const* s)
    {
        if(strm_)
            strm_ << s;
    }

public:
    explicit
    logstream_buf(beast::Journal::Stream const& strm)
        : strm_(strm)
    {
    }

    ~logstream_buf()
    {
        sync();
    }

    int
    sync() override
    {
        write(this->str().c_str());
        this->str("");
        return 0;
    }
};

} 

template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class basic_logstream
    : public std::basic_ostream<CharT, Traits>
{
    typedef CharT                          char_type;
    typedef Traits                         traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;

    detail::logstream_buf<CharT, Traits> buf_;
public:
    explicit
    basic_logstream(beast::Journal::Stream const& strm)
        : std::basic_ostream<CharT, Traits>(&buf_)
        , buf_(strm)
    {
    }
};

using logstream = basic_logstream<char>;
using logwstream = basic_logstream<wchar_t>;

} 

#endif








