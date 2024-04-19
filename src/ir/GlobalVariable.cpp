#include <ir/GlobalVariable.hpp>

GlobalVariable::GlobalVariable(p_symbol_var p_var)
    : GlobalValue(p_var)
{
    is_const = p_var->is_const;
    if (p_var->p_init == nullptr)
    {
        is_init = false;
        init = nullptr;
        return;
    }

    switch (p_var->p_type->basic)
    {
    case type_i32:
        init = new ConstantI32(p_var->p_init->memory->i);
        break;
    case type_f32:
        init = new ConstantF32(p_var->p_init->memory->f);
        break;
    default:
        assert(0);
        break;
    }
}