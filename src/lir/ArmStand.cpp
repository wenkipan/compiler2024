#include "../../include/lir/ArmStand.hpp"

#define ror(value, bits) ((value >> bits) | (value << (sizeof(value) * 8 - bits)))

bool is_legal_rotate_imme(int a)
{
    if (a == 0)
        return true;
    if (a > imm_12_max)
        return false;
    if (a < 0)
    {
        uint32_t neg = -a - 1;
        return is_legal_rotate_imme(neg);
    }
    uint32_t trans = a;
    uint32_t window = ~imm_8_max;
    for (size_t i = 0; i < 16; i++)
    {
        if (!(window & trans))
            return true;
        window = ror(window, 2);
    }
    return false;
}

bool is_legal_ldr_str_imme(int a)
{
    // careful, you need write specific symbol when neg
    // 格式：LDR <Rt>, [<Rn>, #+/-<imm12>]! 和 STR <Rt>, [<Rn>, #+/-<imm12>]!
    if (a < imm_12_max && a > -imm_12_max)
        return true;
    return false;
}