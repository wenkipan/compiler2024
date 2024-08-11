#include "../../include/ir_opt/Manager.hpp"

void Manager::Yi(int YYY)
{
    FuncRun<SCCP>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();

    if (YYY == 1)
    {
        run<GAD>();
        FuncRun<SimplifyCFG>();
    }
}

void Manager::PassManager(bool is_opt)
{
    if (!is_opt)
    {
        NECC();
        return;
    }

    int n = 5;
    for (int i = 0; i < n; i++)
    {
        if (i == 0 || (i >= 3 && i & 1))
        {
            run<GlobalVariableopt>();
            FuncRun<SimplifyCFG>();
            FuncRun<DCE>();
            run<Mem2Reg>();
        }

        //  printModule();
        FuncRun<SimplifyCFG>();
        FuncRun<SCCP>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        printModule();
        FuncRun<GVN>();
        FuncRun<DCE>();
        FuncRun<DFE>();
        FuncRun<SimplifyCFG>();
        FuncRun<GCM>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        printModule();
        run<DeadParamElimate>();

        //  FuncRun<THBalancing>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        Yi(i);
        if (i & 0)
            run<Inline>();
    }

    Finish();
}

void Manager::Finish()
{
    FuncRun<SCCP>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();
    printModule();
    FuncRun<GVN>();
    FuncRun<DFE>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();
    FuncRun<GCM>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();

    run<DeadParamElimate>();
    FuncRun<THBalancing>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();

    FuncRun<SCCP>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();
}

void Manager::NECC()
{

    int n = 1;
    for (int i = 0; i < n; i++)
    {
        if (i == 0 || (i >= 3 && i & 1))
        {
            run<GlobalVariableopt>();
            FuncRun<SimplifyCFG>();
            FuncRun<DCE>();
            run<Mem2Reg>();
        }

        //  printModule();
        FuncRun<SimplifyCFG>();
        FuncRun<SCCP>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        printModule();
        FuncRun<GVN>();
        FuncRun<DCE>();
        FuncRun<DFE>();
        FuncRun<SimplifyCFG>();
        FuncRun<GCM>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        printf("----GVN\n");
        printModule();
        run<DeadParamElimate>();

        //  FuncRun<THBalancing>();
        FuncRun<DCE>();
        FuncRun<SimplifyCFG>();
        run<Inline>();
    }

    FuncRun<SCCP>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();
    printModule();
    FuncRun<GVN>();
    FuncRun<DCE>();
    FuncRun<DFE>();
    FuncRun<SimplifyCFG>();
    FuncRun<GCM>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();

    run<DeadParamElimate>();
    FuncRun<THBalancing>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();

    FuncRun<SCCP>();
    FuncRun<DCE>();
    FuncRun<SimplifyCFG>();
}

Manager::Manager(Module *_module)
    : p_module(_module)
{
    p_module->GenerSet();
}

Manager::~Manager()
{
    delete p_module;
}