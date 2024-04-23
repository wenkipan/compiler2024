#pragma once
#include <ir/ir.hpp>
#include <ir_opt/Mem2Reg.hpp>
#include <ir_opt/SCCP.hpp>

class Pass
{
    Module *module;

public:
    // get_func_vector();
    void run()
    {
        // for(func:funcs)
        //      mem2Reg.run(func);
    }
    void run2()
    {
        // mem2reg
        // inline trans
        // SCCP
    }
};