#include <ir/Value.hpp>

Value::Value()
{
    type = nullptr;
}

Value::Value(TypeEnum basic_type)
{
    type = new Type(basic_type);
}

Value::Value(Type *type)
{
    type = new Type(type);
}

Value::Value(p_symbol_var p_var)
{
    if (list_head_alone(&(p_var->p_type)->array))
        type = new Type(p_var->p_type->basic);
    else
        type = new ArrayType(p_var->p_type);
}

Value::Value(p_symbol_func p_func)
{
    type = new Type(p_func->ret_type);
}

Type *Value::get_type()
{
    return type;
}

void Value::value_list_push_back(Edge *edge)
{
    value_list->emplace_back(edge);
}