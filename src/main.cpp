#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <program/program.hpp>
#include <frontend/use.hpp>
#include <ir/ir.hpp>
#include <ir_opt/Manager.hpp>
#include <backend/arm/ArmGen.hpp>

int main(int argc, char *argv[])
{
    freopen("in.txt", "r", stdin);
    freopen("out.txt", "w", stdout);
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
    // manager->run<GlobalVariableopt>();
    manager->FuncRun<SimplifyCFG>();
    manager->FuncRun<DCE>();
    manager->run<Mem2Reg>();
    // manager->printModule();
    manager->FuncRun<SimplifyCFG>();
    for (int i = 0; i < 3; i++)
    {
        manager->FuncRun<SCCP>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        manager->FuncRun<GVN>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        manager->FuncRun<GCM>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        manager->run<DeadParamElimate>();
        manager->FuncRun<THBalancing>();
        manager->FuncRun<DCE>();
        manager->FuncRun<SimplifyCFG>();
        // manager->run<Inline>();
        manager->printModule();
    }
    manager->printModule();
    manager->run<immeFloatToLoad>();
    manager->run<LargeToGlobal>();
    manager->FuncRun<GEPToALU>();
    printf("GEPTO__________\n");
    manager->printModule();
    printf("________mod___\n");
    manager->FuncRun<modTosubmul>();
    manager->printModule();
    manager->run<immeIntTomove>();
    printf("int");
    manager->printModule();
    fflush(stdout);

    ArmGen backend;
    backend.run(manager->get_module());
    ArmModule *am = backend.get_arm();
    am->print(0);
    delete am;
    // 2lir
    // module->lowerIR();
    // module->print();

    delete manager;
    return 0;
}
