#include <backend/rv/RVstand.hpp>

bool is_legal_rv_imme(int a)
{
    return -2048 < a && a < 2047;
}