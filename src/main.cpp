#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../include/program/program.hpp"
#include "../include/frontend/use.hpp"
#include "../include/ir/ir.hpp"
#include "../include/ir_opt/Manager.hpp"
#include "../include/backend/arm/ArmGen.hpp"

int main(int argc, char *argv[])
{
    // freopen("in.txt", "r", stdin);
    //      freopen("out.txt", "w", stdout);
    char *in_file = NULL, *out_file = NULL;
    std::string Infile, Outfile;
    bool is_opt = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && !strcmp(argv[i], "-o"))
        {
            assert(Outfile.empty());
            assert(++i < argc);
            assert(argv[i][0] != '-');
            out_file = argv[i];
            Outfile = std::string(argv[i], strlen(argv[i]));
            continue;
        }
        if (argv[i] && !strcmp(argv[i], "-O1"))
        {
            assert(!is_opt);
            is_opt = true;
            continue;
        }
        if (argv[i] && !strcmp(argv[i], "-S"))
        {
            continue;
        }
        assert(Infile.empty());
        assert(argv[i][0] != '-');
        in_file = argv[i];
        Infile = std::string(argv[i], strlen(argv[i]));
    }

    // gen ir
    Manager *manager = new Manager(new Module(Infile, Outfile));
    p_program p_program = frontend_trans(in_file, out_file, manager->get_module());
    // p_program->program_variable_print();
    delete p_program;
    // IR
    // manager->printModule("O0");

    int n = 5;
    if (!is_opt)
        n = 5;
    for (int i = 0; i < n; i++)
    {
        if (i == 0 || (i >= 3 && i & 1))
        {
            manager->run<GlobalVariableopt>();
            manager->FuncRun<SimplifyCFG>();
            manager->FuncRun<DCE>();
            manager->run<Mem2Reg>();
        }

        // manager->printModule();
        manager->FuncRun<SimplifyCFG>();
        manager->FuncRun<SCCP>();
        printf("-------------------\n");
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        printf("---------SCCP--------\n");
        if (i == n - 1)
        {
            manager->run<MAD>();
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            manager->printModule();
            puts("Fullunroll");
            manager->run<loopFullunroll>();
            manager->printModule();
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            puts("SSSSSSSS");
            manager->printModule();
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            puts("YYYYYYYYY");
            manager->printModule();
            manager->run<MAD>();
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            manager->printModule();
            manager->run<MAD>();
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            manager->printModule();
        }
        manager->FuncRun<GVN>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        printf("-------------------\n");
        manager->FuncRun<GCM>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        printf("----------GCM---------\n");
        manager->printModule();
        manager->run<DeadParamElimate>();
        printf("-------------------\n");

        if (i == n - 1 || i & 1)
        {
            manager->run<LoopDrop>();
        }
        if (i == 0)
        {
            manager->FuncRun<SCCP>();
            manager->FuncRun<DCE>();
            manager->FuncRun<SimplifyCFG>();
            manager->run<loopYYY>();
            manager->run<LoopDrop>();
        }

        manager->FuncRun<THBalancing>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        manager->run<Inline>();
    }

    manager->FuncRun<SCCP>();
    printf("-------------------\n");
    manager->FuncRun<DCE>();
    manager->FuncRun<SimplifyCFG>();

    printf("lir\n");
    manager->run<immeFloatToLoad>();
    manager->run<LargeToGlobal>();
    manager->FuncRun<GVtoA>();

    manager->FuncRun<GEPToALU>();
    manager->FuncRun<modTosubmul>();
    // manager->FuncRun<Peekhole_s>();
    // lir_opt
    // manager->FuncRun<GVN_l>();
    // manager->FuncRun<DCE>();
    // manager->FuncRun<SimplifyCFG>();
    // manager->FuncRun<GCM>();
    // manager->FuncRun<DCE>();
    // manager->FuncRun<SimplifyCFG>();
    manager->run<immeIntTomove>();
    fflush(stdout);

    ArmGen backend;
    backend.run(manager->get_module());
    ArmModule *am = backend.get_arm();
    am->print(0);
    delete am;

    delete manager;
    return 0;
}
