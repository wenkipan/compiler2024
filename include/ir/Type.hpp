#pragma once
#include <string>
#include <vector>

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
    Type(TypeEnum type_enum);
    Type(Type *p_type);
    Type(basic_type mtype);

    bool isVoid();
    bool isStr();
    bool isI32();
    bool isF32();

    // cout<<Type(specific)
    virtual explicit operator std::string() const
    {
        return "!type!";
    }
};

class ArrayType : public Type
{
    TypeEnum base_type;

    int size;
    std::vector<int> dims;

public:
    ArrayType(p_symbol_type p_type);
};