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
    type = new Type(basic_type);
}

Value::Value(Type *_type, bool _ele) // gep and load
    : value_list(new std::vector<Edge *>),
      user_list(new std::vector<Edge *>)
{
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
        auto tmp = edge->get_user()->get_value_list();
        tmp->erase(remove(tmp->begin(), tmp->end(), edge), tmp->end());
        delete edge;
    }
    user_list->clear();
    value_list->clear();
}
