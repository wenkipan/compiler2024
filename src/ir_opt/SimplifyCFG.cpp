#include <ir_opt/SimplifyCFG.hpp>

void SimplifyCFG::run()
{
    elimate_no_predesessor_block();
    merge_single_predecessor_block();
    eliminate_single_br_block();
    eliminate_single_predecessor_phi();
}
void SimplifyCFG::elimate_no_predesessor_block()
{
    std::unordered_set<BasicBlock *> live_blocks;
    std::queue<BasicBlock *> worklist;
    worklist.push(function->get_entryBB());
    while (!worklist.empty())
    {
        BasicBlock *cur = worklist.front();
        worklist.pop();

        if (live_blocks.find(cur) != live_blocks.end())
            continue;
        live_blocks.emplace(cur);

        for (auto edge : *cur->get_user_list())
        {
            worklist.push((BasicBlock *)edge->get_user());
        }
    }
    for (auto BB : *function->get_blocks())
    {
        if (live_blocks.find(BB) == live_blocks.end())
        {
            worklist.push(BB);
        }
    }
    while (!worklist.empty())
    {
        BasicBlock *BB = worklist.front();
        worklist.pop();
        BB->drop();
    }
}
void SimplifyCFG::eliminate_single_predecessor_phi()
{
    for (auto BB : *function->get_blocks())
    {
        for (auto p : *BB->get_phinodes())
        {
            if (p->get_value_list()->size() == 1)
                philist.push(p);
        }
    }
    while (!philist.empty())
    {
        PHINode *phi = philist.front();
        philist.pop();
        assert(phi->get_value_list()->size() == 1);
        Value *v = phi->get_value_list()->at(0)->get_val();
        // drop single edge
        phi->get_value_list()->at(0)->drop();
        for (auto useredge : *phi->get_user_list())
        { // v   ->  useredge->get_user()
            v->user_list_push_back(useredge);
            useredge->set_val(v);
        }
        phi->get_user_list()->clear();
        phi->drop();
    }
}