#include <backend/rv/RVstand.hpp>

bool is_legal_i_s_imme_rv(int a)
{
    return -2048 < a && a < 2047;
}

bool rv_is_r_reg(int no)
{
    return no >= t0 && no <= t6;
}
bool rv_is_f_reg(int no)
{
    return no >= ft0 && no <= ft11;
}