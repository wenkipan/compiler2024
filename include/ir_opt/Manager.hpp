#pragma once

#include <ir/ir.hpp>
#include <ir_opt/DCE.hpp>
#include <ir_opt/Loop.hpp>
#include <ir_opt/Mem2Reg.hpp>
#include <ir_opt/SCCP.hpp>
#include <ir_opt/DCE.hpp>
#include <ir_opt/SimplifyCFG.hpp>
#include <ir_opt/GlobalVariableopt.hpp>
#include <ir_opt/GVN.hpp>
#include <ir_opt/GCM.hpp>

#include <lir/LargeToGlobal.hpp>
#include <ir_opt/SCEV.hpp>
#include <ir_opt/loopVarReduce.hpp>
#include <ir_opt/loopVarMove.hpp>
#include <ir_opt/loopDrop.hpp>
#include <ir_opt/loopArgDrop.hpp>
#include <ir_opt/THBalancing.hpp>
#include <ir_opt/Inline.hpp>

#include <lir/ArmStand.hpp>
#include <ir_opt/uselessBBDel.hpp>
#include <ir_opt/LivenessAnalysis.hpp>
#include <ir_opt/RegisterAlloc.hpp>
#include <ir_opt/SSARegisterAlloc.hpp>
#include <ir_opt/loopunroll.hpp>
#include <ir_opt/looptimes.hpp>
#include <ir_opt/loopunswitch.hpp>
#include <ir_opt/CSE.hpp>
#include <ir_opt/ALS.hpp>
#include <ir_opt/DPE.hpp>
#include <ir_opt/MAD.hpp>
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