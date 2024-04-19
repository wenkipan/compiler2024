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
