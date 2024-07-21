#include "../../include/ir_opt/BitVector.hpp"

bit_vector::bit_vector()
{
    num = 0;
}

bit_vector::bit_vector(int n)
{
    num = n;
    bits.resize((num - 1) / 32 + 1);
}

bool bit_vector::at(int id)
{
    if (id < 0 || id >= num)
        assert(0);
    return bits[id >> 5] >> (id & 31) & 1;
}

void bit_vector::set(int id, bool x)
{
    if (id < 0 || id >= num)
        assert(0);
    bits[id >> 5] |= (1u << (id & 31));
    if (!x)
        bits[id >> 5] ^= (1u << (id & 31));
}

bit_vector &bit_vector::operator=(const bit_vector &bv)
{
    this->bits = bv.bits;
    this->num = bv.num;
    return *this;
}

bit_vector bit_vector::operator&(bit_vector bv)
{
    if (num != bv.num)
        assert(0);
    int len = bits.size();
    for (int i = 0; i < len; i++)
        bv.bits[i] &= bits[i];
    return bv;
}

bit_vector bit_vector::operator|(bit_vector bv)
{
    if (num != bv.num)
        assert(0);
    int len = bits.size();
    for (int i = 0; i < len; i++)
        bv.bits[i] |= bits[i];
    return bv;
}

bit_vector bit_vector::operator^(bit_vector bv)
{
    if (num != bv.num)
        assert(0);
    int len = bits.size();
    for (int i = 0; i < len; i++)
        bv.bits[i] ^= bits[i];
    return bv;
}

bit_vector bit_vector::operator~()
{
    bit_vector ret(num);
    int len = bits.size();
    for (int i = 0; i < len; i++)
        ret.bits[i] = ~bits[i];
    if (num & 31u)
        ret.bits[len - 1] = bits[len - 1] ^ ((1u << (num & 31u)) - 1u);
    return ret;
}

bool bit_vector::operator==(bit_vector bv)
{
    if (num != bv.num)
        return false;
    int len = bits.size();
    for (int i = 0; i < len; i++)
        if (bits[i] != bv.bits[i])
            return false;
    return true;
}

bool bit_vector::operator!=(bit_vector bv)
{
    return !(*this == bv);
}