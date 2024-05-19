#pragma once

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <ir_opt/DomTree.hpp>
#include <ir_opt/Loop.hpp>

class LCSSA
{
    Loop_Analysis LA;
    void AnalysisForLoop(Loop *p_loop);
    void AnalysisForFunc(Function *p_func);
    void replaceUsesOutofLoop(Instrution *Ins, Value *RepVal, Loop *loop);

public:
    void PassRun(Module *p_module);
};