#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <program/program.hpp>
#include <frontend/use.hpp>
#include <ir/ir.hpp>
#include <ir_opt/Mem2Reg.hpp>

int main(int argc, char *argv[])
{
    freopen("std.in", "r", stdin);
    freopen("std.out", "w", stdout);
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
    Module *module = new Module(Infile, Outfile);
    module->GenerSet();
    p_program p_program = frontend_trans(in_file, out_file, module);
    p_program->program_variable_print();
    delete p_program;
    module->print();
    for (Function *Func : (*module->get_funcs()))
    {
        if (Func->get_isExternal())
            continue;
        Mem2Reg mem2reg;
        mem2reg.run(Func);
    }
    
    module->print();
    delete module;

    // // into ssa
    // ir_simplify_cfg_pass(p_program);
    // mem2reg_program_pass(p_program);
    // // deadcode elimate
    // ir_deadcode_elimate_pass(p_program, true);

    // do
    // {
    //     // optimize - need keep block information
    //     ir_opt_copy_propagation(p_program);
    //     ir_opt_sccp(p_program);
    //     ir_opt_gcm(p_program);

    //     // deadcode elimate
    //     ir_deadcode_elimate_pass(p_program, true);
    // } while (0);

    // // shared lir trans
    // share_lir_trans_pass(p_program);

    // // arm lir trans
    // arm_lir_trans_pass(p_program);
    // set_cond_pass(p_program);
    // reg_alloca_pass(alloca_color_graph, 13, 32, p_program);
    // arm_trans_after_pass(p_program);
    // set_cond_pass(p_program);
    // critical_edge_cut_pass(p_program);

    // arm_codegen_pass(p_program);

    // // drop ir
    // program_ir_print(p_program);
    // program_drop(p_program);
    return 0;
}
