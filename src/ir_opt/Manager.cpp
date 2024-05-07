#include <ir_opt/Manager.hpp>

Manager::Manager(Module *_module)
    : p_module(_module)
{
    p_module->GenerSet();
}

Manager::~Manager()
{
    delete p_module;
}