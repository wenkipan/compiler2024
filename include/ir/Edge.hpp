#pragma once
#include <ir/Value.hpp>
// class Value;
class Edge
{
    Value *value;
    Value *user;

public:
    Edge(Value *_user, Value *_val);

    void set_val(Value *_val) { value = _val; }
    void set_user(Value *_val) { user = _val; }

    Value *get_val() { return value; }
    Value *get_user() { return user; }
    void set_val(Value *RepVal) { value = RepVal; }
    void set_user(Value *RepUser) { user = RepUser; }
    void drop();
};
