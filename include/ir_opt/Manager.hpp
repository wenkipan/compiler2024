#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/DCE.hpp"
#include "../../include/ir_opt/Loop.hpp"
#include "../../include/ir_opt/Mem2Reg.hpp"
#include "../../include/ir_opt/SCCP.hpp"
#include "../../include/ir_opt/DCE.hpp"
#include "../../include/ir_opt/SimplifyCFG.hpp"
#include "../../include/ir_opt/GlobalVariableopt.hpp"
#include "../../include/ir_opt/GVN.hpp"
#include "../../include/ir_opt/GCM.hpp"

#include "../../include/lir/LargeToGlobal.hpp"
#include "../../include/ir_opt/SCEV.hpp"
#include "../../include/ir_opt/loopVarReduce.hpp"
#include "../../include/ir_opt/loopVarMove.hpp"
#include "../../include/ir_opt/loopDrop.hpp"
#include "../../include/ir_opt/THBalancing.hpp"
#include "../../include/ir_opt/Inline.hpp"
#include "../../include/ir_opt/loopunswitch.hpp"
#include "../../include/ir_opt/loopArgDrop.hpp"
#include "../../include/ir_opt/loopFullunroll.hpp"
#include "../../include/ir_opt/ALS.hpp"
#include "../../include/ir_opt/MAD.hpp"
#include "../../include/ir_opt/GAD.hpp"
#include "../../include/ir_opt/loop.hpp"

#include "../../include/lir/ArmStand.hpp"
#include "../../include/ir_opt/uselessBBDel.hpp"
#include "../../include/ir_opt/LivenessAnalysis.hpp"
#include "../../include/ir_opt/SSARegisterAlloc.hpp"
#include "../../include/ir_opt/DPE.hpp"
#include "../../include/ir_opt/DFE.hpp"

class Module;

class Manager
{
    Module *p_module;

public:
    Manager(Module *_module);

    Module *get_module() { return p_module; }
    void printModule() { p_module->print(); }
    void printModule(std::string _sub) { p_module->print(_sub); }

    void NECC();

    void Finish();
    void Yi(int cnt);
    void LoopOpt(int YYY);

    void PassManager(bool is_opt);

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