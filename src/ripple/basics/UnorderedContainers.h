

#ifndef RIPPLE_BASICS_UNORDEREDCONTAINERS_H_INCLUDED
#define RIPPLE_BASICS_UNORDEREDCONTAINERS_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/hash/xxhasher.h>
#include <unordered_map>
#include <unordered_set>



namespace ripple
{


template <class Key, class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hash_map = std::unordered_map <Key, Value, Hash, Pred, Allocator>;

template <class Key, class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hash_multimap = std::unordered_multimap <Key, Value, Hash, Pred, Allocator>;

template <class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hash_set = std::unordered_set <Value, Hash, Pred, Allocator>;

template <class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hash_multiset = std::unordered_multiset <Value, Hash, Pred, Allocator>;


using strong_hash = beast::xxhasher;

template <class Key, class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hardened_hash_map = std::unordered_map <Key, Value, Hash, Pred, Allocator>;

template <class Key, class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hardened_hash_multimap = std::unordered_multimap <Key, Value, Hash, Pred, Allocator>;

template <class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hardened_hash_set = std::unordered_set <Value, Hash, Pred, Allocator>;

template <class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hardened_hash_multiset = std::unordered_multiset <Value, Hash, Pred, Allocator>;

} 

#endif








