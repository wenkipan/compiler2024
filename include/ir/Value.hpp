#pragma once

#include <ir/Type.hpp>
#include <ir/Edge.hpp>

class Value
{
    Type *type;
    int ID;
    EdgeList *value_list;
    EdgeList *user_list;

public:
    Value();
    Value(Type type);
};
