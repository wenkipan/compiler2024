#include <ir/GlobalVariable.hpp>
#include <iostream>

GlobalVariable::GlobalVariable(p_symbol_var p_var)
    : GlobalValue(p_var),
      is_init(true), is_const(p_var->is_const), init(nullptr)
{
    if (p_var->p_init == nullptr)
    {
        is_init = false;
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

void GlobalVariable::print()
{

    this->get_type()->print();
    putchar(' ');
    std::cout << this->get_name();
    putchar('\n');
}

GlobalVariable::~GlobalVariable()
{
    if (init)
        delete init;
}