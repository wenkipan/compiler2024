#include <ir/Edge.hpp>

Edge::Edge(Value *_user, Value *_val)
    : value(_val), user(_user)
{
}