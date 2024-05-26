#include <ir_opt/SimplifyCFG.hpp>
#include <system_error>
#include <unordered_map>

bool SimplifyCFG::run(Function *f)
{
    function = f;
    bool deal = false;
    elimate_no_predesessor_block();
    eliminate_single_predecessor_phi();
    // eliminate_single_br_block(f);
    deal = deal | merge_single_predecessor_block();
    return deal;
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
            worklist.push((BasicBlock *)edge->get_user());
    }
    // reuse worklist
    for (auto BB : *function->get_blocks())
    {
        if (live_blocks.find(BB) == live_blocks.end())
            worklist.push(BB);
    }
    while (!worklist.empty())
    {
        BasicBlock *BB = worklist.front();
        worklist.pop();
        BB->drop();
    }
}
bool SimplifyCFG::merge_single_predecessor_block()
{
    std::queue<BasicBlock *> deal;
    for (auto BB : *function->get_blocks())
    {
        if (BB->get_value_list()->size() == 1)
        {
            BasicBlock *pred = (BasicBlock *)BB->get_value_list()->at(0)->get_val();
            if (pred->get_user_list()->size() == 1)
                deal.push(BB);
        }
    }
    bool flag = false;
    while (!deal.empty())
    {
        flag = true;
        BasicBlock *BB = deal.front();
        deal.pop();

        BasicBlock *pred = (BasicBlock *)BB->get_value_list()->at(0)->get_val();
        assert(pred->get_user_list()->size() == 1);
        assert(pred->get_last_instrution()->isJmp());

        // instr in BB
        pred->get_instrs()->pop_back();
        for (auto instr : *BB->get_instrs())
        {
            instr->set_parent(pred);
            pred->Ins_pushBack(instr);
        }
        BB->get_instrs()->clear();

        // phi in BB
        assert(BB->get_phinodes()->empty());

        // for phi of BBsucc
        for (auto succedge : *BB->get_user_list())
        {
            BasicBlock *succ = (BasicBlock *)succedge->get_user();
            for (auto succphi : *succ->get_phinodes())
            {
                // succphi->print();
                succphi->get_valueMap()->emplace(pred, succphi->get_valueMap()->find(BB)->second);
                succphi->get_valueMap()->erase(BB);
            }
        }
        // for BB's edge
        pred->get_user_list()->at(0)->drop(); // also dealed BB value_list
        for (auto edge : *BB->get_user_list())
        {
            edge->set_val(pred);
        }
        BB->get_user_list()->clear();
        // reset retBB if necessary
        if (BB == BB->get_func()->get_retBB())
            BB->get_func()->set_retBB(pred);
        // isolate and empty BB
        BB->drop();
    }
    return flag;
}
void SimplifyCFG::eliminate_single_br_block(Function *f)
{
    std::queue<BasicBlock *> work;
    for (auto BB : *f->get_blocks())
    {
        auto instrs = *BB->get_instrs();
        if (instrs.size() == 1 && instrs[0]->isJmp())
        {
            assert(BB->get_user_list()->size() == 1);
            work.emplace(BB);
        }
    }
    // change to the pre head queue you created
    while (!work.empty())
    {
        BasicBlock *BB = work.front();
        work.pop();
        BasicBlock *succ = (BasicBlock *)BB->get_user_list()->at(0)->get_user();
        // for total empty BB
        if (BB->get_phinodes()->size() == 0)
        {
            for (auto preedge : *BB->get_value_list())
            {
                preedge->set_user(succ);
                for (auto succphi : *succ->get_phinodes())
                {
                    auto it = succphi->get_valueMap()->find(BB);
                    if (it != succphi->get_valueMap()->end())
                    {
                        succphi->addIncoming(it->second->get_val(), (BasicBlock *)preedge->get_val());
                    }
                }
            }
            BB->get_value_list()->clear();
            BB->drop();
            continue;
        }

        std::unordered_map<PHINode *, PHINode *> pair;
        for (auto BBphi : *BB->get_phinodes())
            if (BBphi->get_user_list()->size() == 1) // for loop anay it can be assert
                for (auto succphi : *succ->get_phinodes())
                    if (BBphi->get_user_list()->at(0)->get_user() == succphi)
                    {
                        pair.emplace(succphi, BBphi);
                        break;
                    }
        if (pair.size() == BB->get_phinodes()->size())
        {
            for (auto succphi : *succ->get_phinodes()) // add preBB income to succphi
            {
                auto it = pair.find(succphi);
                assert(it != pair.end());
                for (auto &kv : *it->second->get_valueMap())
                    succphi->addIncoming(kv.second->get_val(), kv.first);
            }
            for (auto preedge : *BB->get_value_list()) // change preBB br target
                preedge->set_user(succ);
            BB->get_value_list()->clear();
            BB->drop();
        }
    }
}
void SimplifyCFG::eliminate_single_predecessor_phi()
{
    for (auto BB : *function->get_blocks())
    {
        for (auto p : *BB->get_phinodes())
        {
            if (p->get_value_list()->size() == 1)
            {
                philist.push(p);
                // assert(p->get_user_list()->size() == 0);
            }
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
            useredge->set_val(v);
        }
        phi->get_user_list()->clear();
        phi->drop();
    }
}