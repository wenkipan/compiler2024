#include "ir/Instrution.hpp"
#include "ir/Type.hpp"
#include <ir/Value.hpp>
#include <algorithm>
#include <ir/Edge.hpp>
int Value::CurID = 0;

Value::Value()
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>),
      type(nullptr)
{
}

Value::Value(TypeEnum basic_type)
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    if (basic_type == TypeEnum::Ptr)
        type = new Ptr(basic_type);
    else
        type = new Type(basic_type);
}

Value::Value(Type *_type, int is_copy)
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    if (is_copy == 0)
    {
        if (_type->get_type() == TypeEnum::Ptr)
        {
            type = new Ptr(_type);
        }

        else if (_type->get_type() == TypeEnum::Array)
            assert(0);
        else
        {
            type = new Type(_type);
        }
    }
    else if (is_copy == 1)
        type = new Ptr(_type, is_copy);
    else
        assert(0);
}
Value::Value(Type *type)
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    if (type->get_type() == TypeEnum::Ptr)
    {
        this->type = new Ptr(type);
    }
    else if (type->get_type() == TypeEnum::I32 || type->get_type() == TypeEnum::F32)
    {
        this->type = new Type(type->get_type());
    }
    else if (type->get_type() == TypeEnum::Array)
    {
        this->type = new ArrayType(type);
    }
    else if (type->get_type() == TypeEnum::Void)
    {
        this->type = new Type(type->get_type());
    }
    else
    {
        type->print();
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        fflush(stdout);
        assert(0);
    }
}

Value::Value(Type *_type, bool _ele) // gep and load, alloca copy
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    if (_type->get_type() == TypeEnum::I32 || _type->get_type() == TypeEnum::F32)
    {
        assert(!_ele);
        type = new Type(_type->get_type());
        return;
    }

    assert(_type->get_type() == TypeEnum::Ptr);
    Ptr *_ptr = (Ptr *)_type;
    if (!_ele)
    {
        type = new Ptr(_ptr->get_btype());
        return;
    }
    if (_ptr->get_btype()->get_type() == TypeEnum::Array)
    {
        ArrayType *_array = (ArrayType *)((Ptr *)_type)->get_btype();
        if (_array->get_dims()->size() > 1)
            type = new Ptr(_array);
        else
            type = new Ptr(_array->get_basic_type());
    }
    else if (_ptr->get_btype()->get_type() == TypeEnum::Ptr)
    {
        type = new Ptr(((Ptr *)_ptr->get_btype())->get_btype());
    }
    else
    {
        type = new Type(_ptr->get_btype()->get_type());
    }
}

Value::Value(ArrayType *p_array)
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    type = new Ptr(p_array);
}

Value::Value(p_symbol_var p_var) // Alloc
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    type = new Ptr(p_var->p_type);
}
/*
Value::Value(p_symbol_var p_var, basic_type basic) // param
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    if (list_head_alone(&(p_var->p_type)->array))
        type = new Type(p_var->p_type->basic);
    else
        type = new Ptr(p_var->p_type);
}
*/
Value::Value(p_symbol_func p_func)
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
    type = new Type(p_func->ret_type);
}

Type *Value::get_type()
{
    return type;
}

void Value::value_list_push_back(Edge *edge)
{
    value_list->emplace_back(edge);
}

void Value::user_list_push_back(Edge *edge)
{
    user_list->emplace_back(edge);
}

Value::~Value()
{
    delete value_list;
    delete user_list;
    delete type;
}

void Value::drop()
{
    for (Edge *edge : *value_list)
    {
        assert(edge->get_user() == this);
        auto tmp = edge->get_val()->get_user_list();
        tmp->erase(remove(tmp->begin(), tmp->end(), edge), tmp->end());
        delete edge;
    }
    for (Edge *edge : *user_list)
    {
        assert(edge->get_val() == this);
        if (is_a<PHINode>(edge->get_user()))
            for (auto &kv : *((PHINode *)edge->get_user())->get_valueMap())
                if (kv.second == edge)
                {
                    ((PHINode *)edge->get_user())->get_valueMap()->erase(kv.first);
                    break;
                }
        auto tmp = edge->get_user()->get_value_list();
        tmp->erase(remove(tmp->begin(), tmp->end(), edge), tmp->end());
        delete edge;
    }
    user_list->clear();
    value_list->clear();
}
