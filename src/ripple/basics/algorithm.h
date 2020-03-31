

#ifndef RIPPLE_ALGORITHM_H_INCLUDED
#define RIPPLE_ALGORITHM_H_INCLUDED

#include <utility>

namespace ripple {



template <class InputIter1, class InputIter2, class Action, class Comp>
void
generalized_set_intersection(InputIter1 first1, InputIter1 last1,
                             InputIter2 first2, InputIter2 last2,
                             Action action, Comp comp)
{
    while (first1 != last1 && first2 != last2)
    {
        
        if (comp(*first1, *first2))       
            ++first1;                     
        else
        {
            if (!comp(*first2, *first1))  
            {                             
                action(*first1, *first2); 
                ++first1;                 
            }
            ++first2;        
        }
    }
}




template <class FwdIter1, class InputIter2, class Pred, class Comp>
FwdIter1
remove_if_intersect_or_match(FwdIter1 first1, FwdIter1 last1,
                             InputIter2 first2, InputIter2 last2,
                             Pred pred, Comp comp)
{
    
    for (auto i = first1; i != last1;)
    {
        if (first2 == last2 || comp(*i, *first2))
        {
            if (!pred(*i))
            {
                if (i != first1)
                    *first1 = std::move(*i);
                ++first1;
            }
            ++i;
        }
        else  
        {
            if (!comp(*first2, *i))  
                ++i;                 
            ++first2;
        }
    }
    return first1;
}

} 

#endif








