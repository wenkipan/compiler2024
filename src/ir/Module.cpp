#include <ir/Module.hpp>
#include <ir/IRGener.hpp>

#include <iostream>

Module::Module(const std::string &input, const std::string &output)
    : infile(input), outfile(output),
      global_variables(new std::vector<GlobalVariable *>),
      functions(new std::vector<Function *>)
{
}

Module::~Module()
{
    // std::cout << global_variables->size() << std::endl;
    for (GlobalValue *p_Gvar : (*global_variables))
    {
        std::vector<Edge *> *_nw = p_Gvar->get_user_list();
        for (Edge *_edge : (*_nw))
            delete _edge;
        delete p_Gvar;
    }

    for (GlobalValue *p_func : (*functions))
    {
        std::vector<Edge *> *_nw = p_func->get_user_list();
        for (Edge *_edge : (*_nw))
            delete _edge;
        delete p_func;
    }

    delete global_variables;
    delete functions;
}

void Module::GenerSet()
{
    GenFunction::module = this;
}

void Module::Gvar_add(p_symbol_var p_var)
{
    // printf("name:  %s\n", p_var->name);
    if (!p_var->is_global)
        return;
    GlobalVariable *p_value = new GlobalVariable(p_var);
    GenFunction::_global_map->insert(std::pair<p_symbol_var, Value *>(p_var, p_value));
    global_variables->emplace_back(p_value);
}

void Module::func_push_back(Function *p_func)
{

    functions->emplace_back(p_func);
}

std::vector<Function *> *Module::get_funcs()
{
    return functions;
}

void Module::lowerIR()
{
    for (Function *p_func : (*functions))
    {
        std::vector<BasicBlock *> *_blocks = p_func->get_blocks();
        for (BasicBlock *_block : (*_blocks))
        {
            std::vector<Instrution *> *_instrs = _block->get_instrs();
            int n = _instrs->size(), cnt = 0;
            for (int i = 0; i < n + cnt; ++i)
            {
                Instrution *_instr = (*_instrs)[i];
                if (!is_a<GEP>(_instr))
                    continue;
                GEP *_gep = (GEP *)_instr;
                assert(_gep->get_type()->get_type() == TypeEnum::Ptr);
                Ptr *_ptr = (Ptr *)_gep->get_type();
                int _size = 4;
                if (_ptr->get_btype()->get_type() == TypeEnum::Array)
                    _size = ((ArrayType *)_ptr->get_btype())->get_size() << 2;
                ConstantI32 *_int = new ConstantI32(_size);
                p_func->value_pushBack(_int);
                Instrution *p_mul = new Binary(InstrutionEnum::IMUL, _gep->get_offset(), _int, _block);
                p_mul->insertInstr(_block, i);
                ++i, ++cnt;
                Instrution *_add = new Binary(InstrutionEnum::PADD, _gep->get_addr(), p_mul, _block, _gep->get_isele());
                _add->replaceInstr(_block, i);
            }
        }
    }
}

void Module::print()
{
    printf("\n      ---------------IR begin---------------------\n");
    for (GlobalVariable *p_var : (*global_variables))
        p_var->print();
    for (Function *p_func : (*functions))
    {
        if (p_func->get_isExternal())
            continue;
        p_func->ResetID(false);
        p_func->print();
    }
    printf("\n      ---------------IR end-----------------------\n");
}

void Module::print(std::string _sub)
{
    // infile = "sssss";
    int _len = infile.size();
    assert(_len >= 4);
    std::string _outfile = std::string(infile, 0, (_len - 3)) + "_" + _sub + ".ll";
    freopen(const_cast<char *>(_outfile.c_str()), "w", stdout);
    //  printf("source_filename = ");
    //  std::cout << "\"" << _outfile + _sub + ".sy" << "\"" << std::endl;
    puts("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"");
    puts("target triple = \"x86_64-pc-linux-gnu\"\n");
    puts("%struct.timeval = type { i64, i64 }");
    puts("@_sysy_start = dso_local global %struct.timeval zeroinitializer, align 8");
    puts("@_sysy_end = dso_local global %struct.timeval zeroinitializer, align 8");
    puts("@_sysy_l1 = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_l2 = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_h = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_m = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_s = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_us = dso_local global [1024 x i32] zeroinitializer, align 16");
    puts("@_sysy_idx = dso_local global i32 0, align 4\n");
    for (GlobalVariable *p_var : (*global_variables))
        p_var->llvm_print();
    for (Function *p_func : (*functions))
    {
        if (p_func->get_isExternal())
            continue;
        p_func->ResetID(true);
        p_func->print();
    }
    puts("declare i32 @getint(...) #1\n");
    puts("declare i32 @getch() #1\n");
    puts("declare i32 @getarray(ptr noundef) #1\n");
    puts("declare float @getfloat(...) #1\n");
    puts("declare i32 @getfarray(ptr noundef) #1\n");
    puts("declare void @putint(i32 noundef) #1\n");
    puts("declare void @putch(i32 noundef) #1\n");
    puts("declare void @putarray(i32 noundef, ptr noundef) #1\n");
    puts("declare void @putfloat(float noundef) #1\n");
    puts("declare void @putfarray(i32 noundef, ptr noundef) #1\n");
    puts("declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #2\n");
    puts("declare void @putf(ptr noundef, ...) #1\n");
    puts("declare void @_sysy_starttime(i32 noundef) #1\n");
    puts("declare void @_sysy_stoptime(i32 noundef) #1\n");
    _outfile = "output/" + std::string(infile, 0, (_len - 3)) + ".compiler_out";
    freopen(const_cast<char *>(_outfile.c_str()), "a", stdout);
    // freopen("/dev/tty", "w", stdout);
}

/*
p_list_head p_node;
list_for_each(p_node, &p_program->variable)
{
    p_symbol_var p_var = list_entry(p_node, symbol_var, node);
    GlobalVariable *p_value = new GlobalVariable(p_var);
    global_variables->emplace_back(p_value);
}

list_for_each(p_node, &p_program->function)
{
    p_symbol_func p_func = list_entry(p_node, symbol_func, node);
    Function *p_function = new Function(p_func);
    functions->emplace_back(p_function);
}
*/