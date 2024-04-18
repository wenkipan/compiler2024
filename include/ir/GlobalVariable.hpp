#pragma once

#include <ir/GlobalValue.hpp>

class Constant
{
};

class ConstantI32 : public Constant
{
    std::vector<int> i32;

public:
    ConstantI32(p_symbol_init p_init);
};

class ConstantF32 : public Constant
{
    std::vector<float> f32;

public:
    ConstantF32(p_symbol_init p_init);
};

class ConstantStr : public Constant
{
    std::string str;

public:
    ConstantStr(const char *_str);
};

class GlobalVariable : public GlobalValue
{
    bool is_init;
    Constant *init;

public:
    GlobalVariable(p_symbol_var p_var);
};