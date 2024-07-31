#include "../../../include/backend/rv/RVBlockComb.hpp"

#include <cassert>
void RVBlockComb::run(RVModule *m)
{
    for (auto f : m->get_funcs())
    {
        if (f->is_exter())
            continue;
        run(f);
    }
}
void RVBlockComb::merge_sigle_j(RVFunc *f)
{
    std::vector<RVBlock *> merge;
    for (auto BB : RPO(f))
    {
        if (BB->get_instrs().size() == 1)
        {
            BB->print();
            fflush(stdout);
            if (BB->get_last_instr()->get_enum() == RVENUM::rv_j)
            {
                assert(BB->get_user_list().size() == 1);
                RVBlock *succ = (RVBlock *)BB->get_user_list().at(0)->get_user();
                assert(((RVAddr *)BB->get_last_instr()->get_op_at(0))->get_label() == succ);
                merge.push_back(BB);
            }
        }
    }

    for (auto bb : merge)
    {
        RVBlock *succ = (RVBlock *)bb->get_user_list().at(0)->get_user();

        for (auto val : bb->get_value_list())
        {
            auto pred = (RVBlock *)val->get_val();
            for (auto i : pred->get_instrs())
                if (i->get_enum() == RVENUM::rv_j)
                {
                    assert(is_a<RVAddr>(i->get_op_at(0)));
                    if (((RVAddr *)i->get_op_at(0))->get_label() == bb)
                        ((RVAddr *)i->get_op_at(0))->set_label(succ);
                }
                else if (i->get_enum() == RVENUM::rv_b)
                {
                    assert(is_a<RVAddr>(i->get_last_op()));
                    if (((RVAddr *)i->get_last_op())->get_label() == bb)
                        ((RVAddr *)i->get_last_op())->set_label(succ);
                }
        }
        bb->get_user_list().at(0)->drop();
        for (auto val : bb->get_value_list())
            val->set_user(succ);
        bb->get_parent()->erase_block(bb);
        delete bb;
    }
}
void RVBlockComb::run(RVFunc *f)
{
    std::vector<RVBlock *> merge;
    for (auto BB : RPO(f))
    {
        if (BB->get_user_list().size() == 1)
        {
            RVBlock *succ = (RVBlock *)BB->get_user_list().at(0)->get_user();
            if (succ->get_value_list().size() == 1)
            {
                merge.push_back(succ);
            }
        }
    }
    printf("---mergeRVBLOKC\n");
    for (auto succ : merge)
    {
        RVBlock *BB = (RVBlock *)succ->get_value_list().at(0)->get_val();
        BB->print();
        fflush(stdout);
        // change b addr
        for (auto val : BB->get_value_list())
        {
            auto pred = (RVBlock *)val->get_val();
            for (auto i : pred->get_instrs())
                if (i->get_enum() == RVENUM::rv_j)
                {
                    assert(is_a<RVAddr>(i->get_op_at(0)));
                    if (((RVAddr *)i->get_op_at(0))->get_label() == BB)
                        ((RVAddr *)i->get_op_at(0))->set_label(succ);
                }
                else if (i->get_enum() == RVENUM::rv_b)
                {
                    assert(is_a<RVAddr>(i->get_last_op()));
                    if (((RVAddr *)i->get_last_op())->get_label() == BB)
                        ((RVAddr *)i->get_last_op())->set_label(succ);
                }
        }

        auto instrs = BB->get_instrs();
        RVInstr *pos = BB->get_instrs().front();
        for (auto bbin : BB->get_instrs())
        {
            if (bbin == BB->get_last_instr())
            {
                assert(bbin->get_enum() == RVENUM::rv_j);
                assert(is_a<RVAddr>(bbin->get_op_at(0)));
                assert(((RVAddr *)bbin->get_op_at(0))->get_label() == succ);
                delete bbin;
            }
            else
                succ->instr_insert_before(pos, bbin);
        }
        succ->get_value_list().at(0)->drop();
        for (auto e : BB->get_value_list())
            e->set_user(succ);

        BB->get_parent()->erase_block(BB);
        BB->instr_clear();
        delete BB;
    }
    printf("---mergeRVBLOKCend\n");
    merge_sigle_j(f);
    printf("---mergeRVjCend\n");
}