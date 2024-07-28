#include <unordered_set>
#include <iostream>

#include "../../include/lir/LargeToGlobal.hpp"

#define MAX_ARRAY_SIZE 2048
#define MAX_IMMI_SIZE 4096

void LargeToGlobal::run(Module *m)
{
    // large array
    int largearrarycount = 0;
    std::unordered_set<Instrution *> work;
    for (auto f : *m->get_funcs())
        for (auto B : *f->get_blocks())
            for (auto i : *B->get_instrs())
                if (is_a<Alloca>(i))
                {
                    // get large array
                    i->get_type()->print();
                    assert(i->get_type()->get_type() == TypeEnum::Ptr);
                    Type *t = ((Ptr *)i->get_type())->get_btype();
                    if (t->get_type() == TypeEnum::Array && ((ArrayType *)t)->get_size() > MAX_ARRAY_SIZE)
                    {
                        work.emplace(i);
                        std::vector<int> init;
                        // TODO::find all init store(really need?)
                        std::string name = "largearr_" + std::to_string(largearrarycount++);
                        std::cout << name << std::endl;
                        GlobalVariable *GV = new GlobalVariable(i->get_type(), name, init, false);
                        // replace use
                        i->replaceAllUses(GV);
                        i->get_user_list()->clear();
                        m->get_globals()->push_back(GV);
                    }
                }
    // delete alloc
    for (auto k : work)
    {
        k->drop();
    }
}