#include <ir/User.hpp>

User::User(TypeEnum basic_type)
    : Value(basic_type)
{
}

User::User(p_symbol_var p_var)
    : Value(p_var)
{
}

User::User(p_symbol_func p_func)
    : Value(p_func)
{
}