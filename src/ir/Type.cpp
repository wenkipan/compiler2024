#include <ir/Type.hpp>

Type::Type(TypeEnum _type)
    : type(_type)
{
}

Type::Type(Type *p_type)
    : type(p_type->type)
{
}

Type::Type(basic_type mtype)
{
    switch (mtype)
    {
    case type_void:
        type = TypeEnum::Void;
        break;
    case type_f32:
        type = TypeEnum::F32;
        break;
    case type_i32:
        type = TypeEnum::I32;
        break;
    default:
        assert(0);
        break;
    }
}

TypeEnum Type::get_type()
{
    return type;
}

void Type::reset(TypeEnum _type)
{
    type = _type;
}

ArrayType::ArrayType(p_symbol_type p_type)
    : Type(TypeEnum::Array)
{
    basic_type mtype = p_type->basic;
    switch (mtype)
    {
    case type_void:
        assert(0);
        break;
    case type_f32:
        base_type = TypeEnum::F32;
        break;
    case type_i32:
        base_type = TypeEnum::I32;
        break;
    default:
        assert(0);
        break;
    }

    size = 1;
    p_list_head p_node;
    list_for_each(p_node, &p_type->array)
    {
        p_symbol_type_array p_arry = list_entry(p_node, symbol_type_array, node);
        dims.emplace_back(p_arry->size);
        size *= p_arry->size;
    }
}