#pragma once

#include <ir/Type.hpp>
#include <ir/Edge.hpp>

#include <symbol/var.hpp>
#include <symbol/func.hpp>

template <typename T>
bool is_a(Value *p_val)
{
    return dynamic_cast<T *>(p_val) != nullptr;
}

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
    Value(Type *type, bool _ele);
    Value(ArrayType *p_array);
    Value(p_symbol_var p_var);
    // Value(p_symbol_var p_var, basic_type basic);
    Value(p_symbol_func p_func);
    virtual ~Value();

    int get_ID() { return ID; }
    void reset_ID(int nw) { ID = nw; }
    virtual void print_ID() { printf("%%%d", ID); };

    Type *get_type();
    void set_type(Type *ty) { type = ty;}
    std::vector<Edge *> *get_value_list() { return value_list; }
    std::vector<Edge *> *get_user_list() { return user_list; }

    virtual void print() { printf("%%%d\n", ID); }

    void value_list_push_back(Edge *edge);
    void user_list_push_back(Edge *edge);
};

class UndefValue : public Value
{
public:
    UndefValue(Type *ty) {set_type(ty);}
};