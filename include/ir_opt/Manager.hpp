#pragma once

#include <ir_opt/DCE.hpp>
#include <ir_opt/Mem2Reg.hpp>

class Module;

class Manager
{
    Module *p_module;

public:
    Manager(Module *_module);

    Module *get_module() { return p_module; }
    void printModule() { p_module->print(); }

    template <typename Pass>
    void run()
    {
        Pass *_pass = new Pass;
        for (Function *Func : (*p_module->get_funcs()))
        {
            if (Func->get_isExternal())
                continue;
            _pass->run(Func);
        }
        delete _pass;
    }

    ~Manager();
};