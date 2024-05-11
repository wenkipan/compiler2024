#include <ir/Constant.hpp>

Constant::Constant(TypeEnum type)
    : User(type)
{
}

ConstantI32::ConstantI32(p_symbol_init p_init)
    : Constant(TypeEnum::Array), i32(p_init->size)
{
    int i = 0;
    for (int &num : i32)
        num = p_init->memory[i++].i;
}

ConstantI32::ConstantI32(I32CONST_t _I32)
    : Constant(TypeEnum::I32), i32(1, _I32)
{
}

ConstantF32::ConstantF32(p_symbol_init p_init)
    : Constant(TypeEnum::Array), f32(p_init->size)
{
    int i = 0;
    for (float &num : f32)
        num = p_init->memory[i++].f;
}

ConstantF32::ConstantF32(F32CONST_t _F32)
    : Constant(TypeEnum::F32), f32(1, _F32)
{
}

ConstantStr::ConstantStr(const char *_str)
    : Constant(TypeEnum::Str), str(_str, strlen(_str))
{
}

void ConstantI32::print_ID()
{
    if (i32.size() == 1)
        printf("%d", i32[0]);
    else
        printf("%%%d", this->get_ID());
}

void ConstantF32::print_ID()
{
    if (f32.size() == 1)
    {
        double nw = f32[0];
        unsigned long long *ptr = (unsigned long long *)&(nw);
        printf("0x%llX", *ptr);
    }

    else
        printf("%%%d", this->get_ID());
}

// drop

Constant::~Constant()
{
}

ConstantI32::~ConstantI32()
{
}

ConstantF32::~ConstantF32()
{
}

ConstantStr::~ConstantStr()
{
}
