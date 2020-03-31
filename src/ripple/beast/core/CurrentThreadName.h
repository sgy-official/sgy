

#ifndef BEAST_CORE_CURRENT_THREAD_NAME_H_INCLUDED
#define BEAST_CORE_CURRENT_THREAD_NAME_H_INCLUDED

#include <boost/optional.hpp>
#include <string>

namespace beast {


void setCurrentThreadName (std::string newThreadName);

/** Returns the name of the caller thread.

    The name returned is the name as set by a call to setCurrentThreadName().
    If the thread name is set by an external force, then that name change
    will not be reported.  If no name has ever been set, then boost::none
    is returned.
*/
boost::optional<std::string> getCurrentThreadName ();

}

#endif








