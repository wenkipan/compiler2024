#pragma once

#include <ir/User.hpp>

class Constant : public User
{
public:
    Constant(TypeEnum type);
};

class ConstantI32 : public Constant
{
    std::vector<int> i32;

public:
    ConstantI32(p_symbol_init p_init);
    ConstantI32(I32CONST_t _I32);
};

class ConstantF32 : public Constant
{
    std::vector<float> f32;

public:
    ConstantF32(p_symbol_init p_init);
    ConstantF32(F32CONST_t _F32);
};

class ConstantStr : public Constant
{
    std::string str;

public:
    ConstantStr(const char *_str);
};