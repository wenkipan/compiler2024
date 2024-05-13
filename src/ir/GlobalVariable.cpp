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
        if (p_var->p_init->size == 1)
            init = new ConstantI32(p_var->p_init->memory->i);
        else
            init = new ConstantI32(p_var->p_init);
        break;
    case type_f32:
        if (p_var->p_init->size == 1)
            init = new ConstantF32(p_var->p_init->memory->f);
        else
            init = new ConstantF32(p_var->p_init);
        break;
    default:
        assert(0);
        break;
    }
}

void GlobalVariable::llvm_print()
{
    assert(this->get_type()->get_type() == TypeEnum::Ptr);
    std::cout << '@' << get_name() << " =  dso_local ";
    if (is_const)
        printf("constant ");
    else
        printf("global ");

    Type *_mtype = ((Ptr *)this->get_type())->get_btype();
    if (_mtype->get_type() == TypeEnum::Array)
    {
        if (!init)
        {
            _mtype->print();
            printf(" zeroinitializer, align 16\n");
        }
        else
        {
            init->llvm_print(_mtype);
        }
    }
    else
    {
        _mtype->print();
        putchar(' ');
        if (init)
            init->print_ID();
        else
        {
            switch (_mtype->get_type())
            {
            case TypeEnum::I32:
                putchar('0');
                break;
            case TypeEnum::F32:
                printf("0.000000e+00");
                break;
            default:
                assert(0);
                break;
            }
        }
        printf(", align 4\n");
    }
}

void GlobalVariable::print()
{
    assert(this->get_type()->get_type() == TypeEnum::Ptr);
    std::cout << '@' << get_name() << " =  dso_local ";
    if (is_const)
        printf("constant ");
    else
        printf("global ");

    Type *_mtype = ((Ptr *)this->get_type())->get_btype();
    if (_mtype->get_type() == TypeEnum::Array)
    {
        _mtype->print();
        printf(" , align 16\n");
    }
    else
    {
        _mtype->print();
        putchar(' ');
        if (init)
            init->print_ID();
        else
        {
            switch (_mtype->get_type())
            {
            case TypeEnum::I32:
                putchar('0');
                break;
            case TypeEnum::F32:
                printf("0.000000e+00");
                break;
            default:
                assert(0);
                break;
            }
        }
        printf(", align 4\n");
    }
}

GlobalVariable::~GlobalVariable()
{
    if (init)
        delete init;
}
