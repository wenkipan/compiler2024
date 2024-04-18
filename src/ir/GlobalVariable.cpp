#include <ir/GlobalVariable.hpp>

ConstantI32::ConstantI32(p_symbol_init p_init)
    : i32(p_init->size)
{
    int i = 0;
    for (int &num : i32)
        num = p_init->memory[i++].i;
}

ConstantF32::ConstantF32(p_symbol_init p_init)
    : f32(p_init->size)
{
    int i = 0;
    for (float &num : f32)
        num = p_init->memory[i++].f;
}

GlobalVariable::GlobalVariable(p_symbol_var p_var)
    : GlobalValue(p_var)
{
    if (p_var->p_init == nullptr)
    {
        is_init = false;
        init = nullptr;
        return;
    }

    switch (p_var->p_type->basic)
    {
    case type_i32:
        init = new ConstantI32(p_var->p_init);
        break;
    case type_f32:
        init = new ConstantF32(p_var->p_init);
        break;
    default:
        assert(0);
        break;
    }
}