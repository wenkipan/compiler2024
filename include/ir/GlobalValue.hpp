
#pragma once
#include <ir/User.hpp>

class GlobalValue : public User
{
    std::string name;

public:
    GlobalValue();
};