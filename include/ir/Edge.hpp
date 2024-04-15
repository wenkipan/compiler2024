#pragma once

#include <vector>

class Value;
class Edge
{
    Value *value;
    Value *user;

public:
    Edge();
};

// linux list?
class EdgeList
{
    std::vector<Edge *> *edgelist;

public:
    EdgeList();
    void edge_push_back();
    void edge_push_front();
};
