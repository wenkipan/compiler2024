#include <lir/immeFloatToLoad.hpp>
#include <iostream>
void immeFloatToLoad::run(Module *m)
{
    int immifcount = 0;
    for (auto f : *m->get_funcs())
    {
        std::vector<Value *> values1(*f->get_Values());
        for (auto val : values1)
        {
            if (val->get_user_list()->empty())
                continue;
            if (is_a<ConstantF32>(val))
            {
                GlobalVariable *GV;
                std::vector<float> vf;
                vf.push_back(((ConstantF32 *)val)->get_32_at(0));
                std::string name = "_immif_" + std::to_string(immifcount++);
                std::cout << name << std::endl;
                Ptr *ptr = new Ptr(((ConstantI32 *)val)->get_type());
                GV = new GlobalVariable(ptr, name, vf, false);
                delete ptr;

                // deal with user
                for (auto useredge : *val->get_user_list())
                {
                    assert(is_a<Instrution>(useredge->get_user()));
                    Instrution *user = (Instrution *)useredge->get_user();
                    user->print();
                    Instrution *l;
                    if (is_a<PHINode>(user)) // phiuser, insert into cfg pre block
                    {
                        // find cfg bb
                        BasicBlock *b = nullptr;
                        for (auto kv : *((PHINode *)user)->get_valueMap())
                        {
                            if (kv.second == useredge)
                                b = kv.first;
                        }
                        assert(b);
                        printf("111!!!!!");
                        b->print();
                        fflush(stdout);
                        l = new Load(GV, true, user->get_parent()); // gv
                        int pos;
                        if (b->get_last_instrution()->isBranch())
                            pos = (int)b->get_instrs()->size() - 2;
                        else if (b->get_last_instrution()->isJmp())
                            pos = (int)b->get_instrs()->size() - 1;
                        else
                            assert(0);
                        std::cout << pos << std::endl;
                        l->insertInstr(b, pos);
                    }
                    else // others
                    {
                        printf("2!!!!!");
                        fflush(stdout);
                        l = new Load(GV, true, user->get_parent());
                        // replace me!
                        auto v = user->get_parent()->get_instrutions();
                        int i = 0;
                        for (auto it = v->begin(); it != v->end(); it++, i++)
                            if ((*it) == user)
                                break;
                        l->insertInstr(l->get_parent(), i);
                    }
                    useredge->set_val(l);
                }
                val->get_user_list()->clear();

                m->get_globals()->push_back(GV);
            }
        }
    }
}