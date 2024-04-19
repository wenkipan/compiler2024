#pragma once

#include <ir/Type.hpp>
#include <ir/Edge.hpp>

#include <symbol/var.hpp>
#include <symbol/func.hpp>

class Value
{

    int ID = CurID++;
    std::vector<Edge *> *value_list;
    std::vector<Edge *> *user_list;
    Type *type;

public:
    static int CurID;

    static void clear_CurID() { CurID = 0; }

    Value();
    Value(TypeEnum basic_type);
    Value(Type *type);
    Value(p_symbol_var p_var);
    Value(p_symbol_var p_var, basic_type basic);
    Value(p_symbol_func p_func);

    Type *get_type();

    void value_list_push_back(Edge *edge);
    void user_list_push_back(Edge *edge);
};
