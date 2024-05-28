#pragma once

#include <ir/ir.hpp>
#include <ir_opt/DCE.hpp>
#include <ir_opt/Loop.hpp>
#include <ir_opt/Mem2Reg.hpp>
#include <ir_opt/SCCP.hpp>
#include <ir_opt/DCE.hpp>
#include <ir_opt/SimplifyCFG.hpp>
#include <ir_opt/GlobalVariableopt.hpp>
class Module;

class Manager
{
    Module *p_module;

public:
    Manager(Module *_module);

    Module *get_module() { return p_module; }
    void printModule() { p_module->print(); }
    void printModule(std::string _sub) { p_module->print(_sub); }

    template <typename Pass>
    void run()
    {
        Pass *_pass = new Pass;
        _pass->PassRun(p_module);
        delete _pass;
    }
    template <typename Pass>
    void FuncRun()
    {
        for (auto func : *p_module->get_funcs())
        {
            if (func->get_isExternal())
                continue;
            Pass pass;
            pass.run(func);
        }
    }
    ~Manager();
};