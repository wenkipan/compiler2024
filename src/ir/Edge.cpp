#include <ir/Instrution.hpp>
#include <ir/Edge.hpp>
#include <algorithm>
Edge::Edge(Value *_user, Value *_val)
    : value(_val), user(_user)
{
    assert(_val && _user);
    value->user_list_push_back(this);
    user->value_list_push_back(this);
}
void Edge::drop()
{
    auto tmp = value->get_user_list();
    tmp->erase(remove(tmp->begin(), tmp->end(), this), tmp->end());

    tmp = user->get_value_list();
    tmp->erase(remove(tmp->begin(), tmp->end(), this), tmp->end());
    delete this;
}
void drop_all_edges(std::queue<Edge *> q)
{
    while (!q.empty())
    {
        Edge *t = q.front();
        q.pop();
        t->drop();
    }
}
void Edge::set_user(Value *RepVal)
{
    user = RepVal;
    if (is_a<PHINode>(user))
    {
        assert(is_a<Instrution>(this->value));
        ((PHINode *)user)->get_valueMap()->emplace(((Instrution *)this->value)->get_parent(), this->value);
    }
    user->value_list_push_back(this);
}
void Edge::set_val(Value *RepVal)
{
    if (is_a<PHINode>(user))
    {
        for (auto kv : *((PHINode *)user)->get_valueMap())
        {
            if (kv.second == this->value)
                kv.second = RepVal;
        }
    }
    value = RepVal;
    value->user_list_push_back(this);
}