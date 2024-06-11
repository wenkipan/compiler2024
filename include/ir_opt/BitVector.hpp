#pragma once

#include <vector>
#include <assert.h>
#include <cstdio>
#include <cstdlib>

class bit_vector
{
    std::vector<unsigned int> bits;
    int num;

public:
    bit_vector();
    bit_vector(int n);
    bool at(int id);
    void set(int id, bool x);
    bit_vector &operator=(const bit_vector &bv);
    bit_vector operator&(bit_vector bv);
    bit_vector operator|(bit_vector bv);
    bit_vector operator^(bit_vector bv);
    bit_vector operator~();
    bool operator==(bit_vector bv);
    bool operator!=(bit_vector bv);
    int size() { return num; }
};