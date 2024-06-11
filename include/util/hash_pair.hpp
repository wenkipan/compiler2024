#pragma once
#include <cstdlib>
#include <utility>

struct pair_hash
{
    template <class T1, class T2>
    size_t operator()(std::pair<T1, T2> const &pair) const
    {
        size_t h1 = std::hash<T1>()(pair.first);
        size_t h2 = std::hash<T2>()(pair.second);
        return h1 ^ h2;
    }
};