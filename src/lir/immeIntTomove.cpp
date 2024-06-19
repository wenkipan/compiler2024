#include <lir/immeIntTomove.hpp>
#include <iostream>

#include <lir/ArmStand.hpp>

void immeIntTomove::run(Module *m)
{
    for (auto f : *m->get_funcs())
    {
        // values1 dont modify
        std::vector<Value *> values1(*f->get_Values());
        for (auto val : values1)
        {
            if (val->get_user_list()->empty())
                continue;
            if ((is_a<ConstantI32>(val) && !is_legal_rotate_imme(((ConstantI32 *)val)->get_32_at(0))))
            {
                // visit dont modify
                auto visit = *val->get_user_list();
                val->get_user_list()->clear();
                for (auto useredge : visit)
                {
                    assert(is_a<Instrution>(useredge->get_user()));
                    Instrution *user = (Instrution *)useredge->get_user();
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
                        // new assign
                        l = new Assign(InstrutionEnum::Assign, val, user->get_parent());
                        int pos;
                        if (b->get_last_instrution()->isBranch())
                            pos = (int)b->get_instrs()->size() - 2;
                        else if (b->get_last_instrution()->isJmp())
                            pos = (int)b->get_instrs()->size() - 1;
                        else
                            assert(0);
                        l->insertInstr(b, pos);
                    }
                    else // others
                    {
                        // new assign
                        l = new Assign(InstrutionEnum::Assign, val, user->get_parent());
                        // right place
                        auto v = user->get_parent()->get_instrutions();
                        int i = 0;
                        for (auto it = v->begin(); it != v->end(); it++, i++)
                            if ((*it) == user)
                                break;
                        l->insertInstr(l->get_parent(), i);
                    }
                    useredge->set_val(l);
                }
            }
        }
    }
}
