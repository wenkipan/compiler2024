#pragma once

class Value;
class Edge
{
    Value *value;
    Value *user;

public:
    Edge(Value *_user, Value *_val);
};
