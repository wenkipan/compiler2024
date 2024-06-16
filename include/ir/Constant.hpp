#pragma once

#include <ir/User.hpp>

class Constant : public User
{
public:
    Constant(TypeEnum type);
    Constant(Type *ty);
    virtual ~Constant() override;
    virtual void llvm_print(Type *_type) { assert(0); };
};

class ConstantI32 : public Constant
{
    std::vector<int> i32;

public:
    ConstantI32(p_symbol_init p_init);
    ConstantI32(I32CONST_t _I32);
    ConstantI32(Type *type, const std::vector<int> &vi);
    ConstantI32(ConstantI32 *ci32);
    ~ConstantI32() override;

    void print_ID() override;
    void llvm_print(Type *_type) override;

    std::vector<int> get_i32() { return i32; }
    int get_32_at(int pos);
};

class ConstantF32 : public Constant
{
    std::vector<float> f32;

public:
    ConstantF32(p_symbol_init p_init);
    ConstantF32(F32CONST_t _F32);
    ConstantF32(Type *type, const std::vector<float> &vi);
    ConstantF32(ConstantF32 *cf32);
    ~ConstantF32() override;

    void print_ID() override;
    void llvm_print(Type *_type) override;

    std::vector<float> get_f32() { return f32; }
    float get_32_at(int pos);
};

class ConstantStr : public Constant
{
    std::string str;

public:
    ConstantStr(const char *_str);
    ~ConstantStr() override;
};