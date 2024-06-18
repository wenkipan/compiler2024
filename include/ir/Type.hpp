#pragma once
#include <string>
#include <vector>

#include <symbol/type.hpp>

enum class TypeEnum
{
    Void,
    Str,
    I32,
    I1,
    F32,
    Ptr,
    Array,
};

class Type
{
    TypeEnum type;

public:
    Type(TypeEnum _type);
    Type(Type *p_type);
    Type(basic_type mtype);

    virtual ~Type();

    bool isVoid();
    bool isStr();
    bool isI32();
    bool isF32();
    bool isArray() { return type == TypeEnum::Array; }

    TypeEnum get_type();

    void reset(TypeEnum _type);

    virtual TypeEnum get_basic_type() { return type; };
    virtual void print();
    // cout<<Type(specific)
    virtual explicit
    operator std::string() const
    {
        return "!type!";
    }
};

class ArrayType : public Type
{
    TypeEnum b_type;

    int size;
    std::vector<int> *dims;

public:
    ArrayType(p_symbol_type p_type);
    ArrayType(ArrayType *p_array);
    ArrayType(Type *_type);
    ~ArrayType() override;

    TypeEnum get_basic_type() override { return b_type; };

    TypeEnum get_basictype() { return b_type; }
    std::vector<int> *get_dims() { return dims; }
    int get_size() { return size; }
    void print() override;
    void print(int pos);
};
class Ptr : public Type
{
    Type *b_type;

public:
    Ptr(TypeEnum _type);
    Ptr(Type *_type);
    Ptr(ArrayType *_type);
    Ptr(p_symbol_type p_var);
    Ptr(Type *_type, int flag);
    Ptr(Ptr *_ptr);
    ~Ptr() override;

    TypeEnum get_basic_type() override { return b_type->get_basic_type(); };
    Type *get_btype() { return b_type; }
    void print_btype() { b_type->print(); };
};
