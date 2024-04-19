#include <ir/Value.hpp>

int Value::CurID = 0;

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

Value::Value(p_symbol_var p_var) // variable
{
    if (list_head_alone(&(p_var->p_type)->array))
        type = new Type(p_var->p_type->basic);
    /*
    {
        if (p_var->p_type->basic == basic_type::type_i32)
            type = new Type(TypeEnum::PtrI32);
        else
            type = new Type(TypeEnum::PtrF32);
    }
    */
    else
        type = new ArrayType(p_var->p_type);
}

Value::Value(p_symbol_var p_var, basic_type basic) // param
{
    if (list_head_alone(&(p_var->p_type)->array))
        type = new Type(p_var->p_type->basic);
    else
    {
        if (basic == basic_type::type_i32)
            type = new Type(TypeEnum::PtrI32);
        else
            type = new Type(TypeEnum::PtrF32);
    }
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

void Value::user_list_push_back(Edge *edge)
{
    user_list->emplace_back(edge);
}