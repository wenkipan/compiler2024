#pragma once

class Value;
class Edge
{
    Value *value;
    Value *user;

public:
    Edge(Value *_user, Value *_val);

    Value *get_val() { return value; }
    Value *get_user() { return user; }
    void modify_val(Value *RepVal) { value = RepVal;}
    void modify_user(Value *RepUser) { user = RepUser;}
};
