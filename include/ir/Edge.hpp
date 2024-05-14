#pragma once
#include <ir/Value.hpp>
#include <queue>
// class Value;
class Edge
{
    Value *value;
    Value *user;

public:
    Edge(Value *_user, Value *_val);

    Value *get_val() { return value; }
    Value *get_user() { return user; }
    void set_val(Value *RepVal);
    void set_user(Value *RepUser);
    void drop();
    friend void drop_all_edges(std::queue<Edge *> q);
};
