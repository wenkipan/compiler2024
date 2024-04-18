#pragma once
#include <string>
#include <vector>
#include <symbol/symbol.hpp>
#include <symbol/type.hpp>
enum class TypeEnum
{
    Void,
    Str,
    I32,
    F32,
    Array,

};

class Type
{
    TypeEnum type;

public:
    Type(basic_type);
    Type(p_symbol_type);
    Type(TypeEnum type_enum);
    Type(Type *p_type);
    Type(basic_type mtype);

    bool isVoid();
    bool isStr();
    bool isI32();
    bool isF32();
    bool isArray();
    // cout<<Type(specific)
    virtual explicit operator std::string() const
    {
        return "!type!";
    }
};

class ArrayType : public Type
{
    TypeEnum base_type;
<<<<<<< HEAD

=======
>>>>>>> 21d6db0d1807a77d4e8040c3e0cdd47cdd05d700
    int size;
    std::vector<int> dims;

public:
<<<<<<< HEAD
    ArrayType(p_symbol_type p_type);
=======
    ArrayType(p_symbol_type);
    void push_back(int i) { dims.push_back(i); }
>>>>>>> 21d6db0d1807a77d4e8040c3e0cdd47cdd05d700
};