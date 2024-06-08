#include <ir/User.hpp>

User::User(TypeEnum basic_type)
    : Value(basic_type)
{
}

User::User(p_symbol_var p_var)
    : Value(p_var)
{
}
/*
User::User(p_symbol_var p_var, basic_type basic)
    : Value(p_var, basic)
{
}
*/
User::User(p_symbol_func p_func)
    : Value(p_func)
{
}
User::User()
    : Value() {}
User::User(Type *p_array, bool _ele)
    : Value(p_array, _ele)
{
}

User::User(Type *p_array, int is_copy)
    : Value(p_array, is_copy)
{
}

User::~User()
{
}