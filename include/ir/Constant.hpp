#pragma once
#include <ir/Value.hpp>

class Constant : public Value
{
};

class ConstantInt : public Constant
{
    int num;
};
class ConstantFloat : public Constant
{
    float num;
};