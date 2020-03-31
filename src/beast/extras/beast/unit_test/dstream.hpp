
#ifndef BEAST_UNIT_TEST_DSTREAM_HPP
#define BEAST_UNIT_TEST_DSTREAM_HPP

#include <boost/config.hpp>
#include <ios>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>

#ifdef BOOST_WINDOWS
#include <boost/detail/winapi/basic_types.hpp>
#endif

namespace beast {
namespace unit_test {

#ifdef BOOST_WINDOWS

namespace detail {

template<class CharT, class Traits, class Allocator>
class dstream_buf
    : public std::basic_stringbuf<CharT, Traits, Allocator>
{
    using ostream = std::basic_ostream<CharT, Traits>;

    bool dbg_;
    ostream& os_;

    template<class T>
    void write(T const*) = delete;

    void write(char const* s)
    {
        if(dbg_)
            ::OutputDebugStringA(s);
        os_ << s;
    }

    void write(wchar_t const* s)
    {
        if(dbg_)
            ::OutputDebugStringW(s);
        os_ << s;
    }

public:
    explicit
    dstream_buf(ostream& os)
        : os_(os)
        , dbg_(::IsDebuggerPresent() != 0)
    {
    }

    ~dstream_buf()
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
    class Traits = std::char_traits<CharT>,
    class Allocator = std::allocator<CharT>
>
class basic_dstream
    : public std::basic_ostream<CharT, Traits>
{
    detail::dstream_buf<
        CharT, Traits, Allocator> buf_;

public:
    
    explicit
    basic_dstream(std::ostream& os)
        : std::basic_ostream<CharT, Traits>(&buf_)
        , buf_(os)
    {
        if(os.flags() & std::ios::unitbuf)
            std::unitbuf(*this);
    }
};

using dstream = basic_dstream<char>;
using dwstream = basic_dstream<wchar_t>;

#else

using dstream = std::ostream&;
using dwstream = std::wostream&;

#endif

} 
} 

#endif






