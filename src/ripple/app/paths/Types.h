

#ifndef RIPPLE_APP_PATHS_TYPES_H_INCLUDED
#define RIPPLE_APP_PATHS_TYPES_H_INCLUDED

namespace ripple {

using AccountIssue = std::pair <AccountID, Issue>;

namespace path {

using NodeIndex = unsigned int;

}

using AccountIssueToNodeIndex = hash_map <AccountIssue, path::NodeIndex>;

} 

#endif








