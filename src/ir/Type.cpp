#include "symbol/symbol.hpp"
#include <ir/Type.hpp>

Type::Type(basic_type bt)
{
    if (bt == type_void)
        type = TypeEnum::Void;
    else if (bt == type_f32)
        type = TypeEnum::F32;
    else if (bt == type_i32)
        type = TypeEnum::I32;
}
Type::Type(p_symbol_type p_type)
{
    if (list_head_alone(&p_type->array))
    {
        basic_type bt = p_type->basic;
        if (bt == type_void)
            type = TypeEnum::Void;
        else if (bt == type_f32)
            type = TypeEnum::F32;
        else if (bt == type_i32)
            type = TypeEnum::I32;
    }
}

ArrayType::ArrayType(p_symbol_type p_type)
    : Type(TypeEnum::Array)
{
    assert(!list_head_alone(&p_type->array));
    basic_type bt = p_type->basic;
    if (bt == type_void)
        base_type = TypeEnum::Void;
    else if (bt == type_f32)
        base_type = TypeEnum::F32;
    else if (bt == type_i32)
        base_type = TypeEnum::I32;
    size = 1;
    p_list_head p_node;
    list_for_each(p_node, &p_type->array)
    {
        symbol_type_array ai = list_entry(p_node, symbol_type_array, node);
        dims.push_back(ai.size);
        size *= ai.size;
    }
}