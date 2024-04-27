#pragma once

#include <ir/User.hpp>

class Constant : public User
{
public:
    Constant(TypeEnum type);
    virtual ~Constant() override;
};

class ConstantI32 : public Constant
{
    std::vector<int> i32;

public:
    ConstantI32(p_symbol_init p_init);
    ConstantI32(I32CONST_t _I32);
    ~ConstantI32() override;

    void print_ID() override;

    std::vector<int> get_i32() { return i32; }
};

class ConstantF32 : public Constant
{
    std::vector<float> f32;

public:
    ConstantF32(p_symbol_init p_init);
    ConstantF32(F32CONST_t _F32);
    ~ConstantF32() override;

    void print_ID() override;

    std::vector<float> get_f32() { return f32; }
};

class ConstantStr : public Constant
{
    std::string str;

public:
    ConstantStr(const char *_str);
    ~ConstantStr() override;
};