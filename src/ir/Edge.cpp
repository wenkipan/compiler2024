#include <ir/Edge.hpp>
#include <algorithm>
Edge::Edge(Value *_user, Value *_val)
    : value(_val), user(_user)
{
}
void Edge::drop()
{
    auto tmp = value->get_user_list();
    tmp->erase(remove(tmp->begin(), tmp->end(), this), tmp->end());

    tmp = user->get_value_list();
    tmp->erase(remove(tmp->begin(), tmp->end(), this), tmp->end());
    delete this;
}
void drop_all_edge(std::queue<Edge *> q)
{
    while (!q.empty())
    {
        Edge *t = q.front();
        q.pop();
        t->drop();
    }
}