#pragma once
#include <string>

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