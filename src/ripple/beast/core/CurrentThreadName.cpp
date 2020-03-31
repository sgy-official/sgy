

#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/beast/core/Config.h>
#include <boost/thread/tss.hpp>
#include <ripple/beast/core/BasicNativeHeaders.h>
#include <ripple/beast/core/StandardIncludes.h>

namespace beast {
namespace detail {

static boost::thread_specific_ptr<std::string> threadName;

void saveThreadName (std::string name)
{
    threadName.reset (new std::string {std::move(name)});
}

} 

boost::optional<std::string> getCurrentThreadName ()
{
    if (auto r = detail::threadName.get())
        return *r;
    return boost::none;
}

} 


#if BEAST_WINDOWS

#include <windows.h>
#include <process.h>
#include <tchar.h>

namespace beast {
namespace detail {

void setCurrentThreadNameImpl (std::string const& name)
{
   #if BEAST_DEBUG && BEAST_MSVC
    struct
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    } info;

    info.dwType = 0x1000;
    info.szName = name.c_str ();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try
    {
        RaiseException (0x406d1388 , 0, sizeof (info) / sizeof (ULONG_PTR), (ULONG_PTR*) &info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {}
   #else
    (void) name;
   #endif
}

} 
} 

#elif BEAST_MAC

#include <pthread.h>

namespace beast {
namespace detail {

void setCurrentThreadNameImpl (std::string const& name)
{
    pthread_setname_np(name.c_str());
}

} 
} 

#else  

#include <pthread.h>

namespace beast {
namespace detail {

void setCurrentThreadNameImpl (std::string const& name)
{
    pthread_setname_np(pthread_self(), name.c_str());
}

} 
} 

#endif  

namespace beast {

void setCurrentThreadName (std::string name)
{
    detail::setCurrentThreadNameImpl (name);
    detail::saveThreadName (std::move (name));
}

} 
























