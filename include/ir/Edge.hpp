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
    void set_val(Value *RepVal) { value = RepVal; }
    void set_user(Value *RepUser) { user = RepUser; }
    void drop();
    friend void drop_all_edge(std::queue<Edge *> q);
};
