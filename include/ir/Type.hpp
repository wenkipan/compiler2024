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
    int size;
    std::vector<int> dims;

public:
    ArrayType(p_symbol_type);
    void push_back(int i) { dims.push_back(i); }
};