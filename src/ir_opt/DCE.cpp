#include "ir/BasicBlock.hpp"
#include "ir/Constant.hpp"
#include "ir/Edge.hpp"
#include "ir/Instrution.hpp"
#include <ir_opt/DCE.hpp>
#include <iostream>
#include <fstream>
void DCE::init()
{
    block_DCE bdce(function);
    bdce.run();
    pdt = new PostDomTree(function);
    for (auto bb : *function->get_blocks())
    {
        // for (auto phi : *bb->get_phinodes())
        // {
        // }
        for (auto instr : *bb->get_instrs())
        {
            // side-effect and terminator
            switch (instr->get_Instrtype())
            {
            case InstrutionEnum::Store:
            case InstrutionEnum::Ret:
            case InstrutionEnum::Call:
                worklist.push(instr);
                live_instr.emplace(instr);
                break;
            default:
                break;
            }
        }
    }
}
void DCE::run()
{
    init();
    do
    {
        // Worklist holds newly discovered live instructions
        // where we need to mark the inputs as live.
        while (!worklist.empty())
        {
            Instrution *instr = worklist.front();
            worklist.pop();

            if (is_live(instr)) // already checked
                continue;
            live_instr.emplace(instr);

            if (!is_live(instr->get_parent()))
            {
                live_blocks.emplace(instr->get_parent());
                newliveblocks.emplace(instr->get_parent());
            }

            for (auto edge : *instr->get_value_list())
            {
                if (is_a<Instrution>(edge->get_val()))
                    worklist.push((Instrution *)edge->get_val());
            }
        }
        // After data flow liveness has been identified, examine which branch
        // decisions are required to determine live instructions are executed.
        mark_live_from_CDG();

    } while (!worklist.empty());

    print();
    elimate();
}
void DCE::mark_live_from_CDG()
{
    // newliveblocks :: add their CDG block terminator to worklist
    for (auto BB : newliveblocks)
    {
        auto CDGpreds = pdt->pdt->DomFsBlock.find(BB)->second;
        for (auto bb : CDGpreds)
            if (bb->get_instrs()->size())
                worklist.push(bb->get_last_instrution());
            else
                assert(bb == pdt->entry);
    }
    newliveblocks.clear();
}
void DCE::elimate()
{
    std::queue<Value *> deletelist;
    std::queue<PHINode *> simplyphilist;
    // elimate dead block and its phi user
    for (auto bb : *function->get_blocks())
    {
        if (!is_live(bb))
        {
            deletelist.push(bb);
            continue;
        }
        // elimate dead instr and phi
        for (auto instr : *bb->get_instrs())
        {
            if (!is_live(instr))
                deletelist.push(instr);
        }
        for (auto phi : *bb->get_phinodes())
        {
            if (!is_live(phi))
                deletelist.push(phi);
            else if (phi->get_value_list()->size() == 1)
            {
                simplyphilist.push(phi);
            }
        }
    }
    while (!deletelist.empty())
    {
        Value *bb = deletelist.front();
        deletelist.pop();
        if (is_a<BasicBlock>(bb))
            ((BasicBlock *)bb)->drop();
        else
        {
            assert(is_a<Instrution>(bb));
            ((Instrution *)bb)->drop();
        }
    }
    while (!simplyphilist.empty())
    {
        PHINode *p = simplyphilist.front();
        simplyphilist.pop();
        simplifyPHI(p);
    }
}
void DCE::simplifyPHI(PHINode *phi)
{
    assert(phi->get_value_list()->size() == 1);
    Value *v = phi->get_value_list()->at(0)->get_val();
    // drop single edge
    phi->get_value_list()->at(0)->drop();
    for (auto useredge : *phi->get_user_list())
    { // v   ->  useredge->get_user()
        v->user_list_push_back(useredge);
        useredge->set_val(v);
    }
    delete phi;
}
void DCE::elimate_block(BasicBlock *bb)
{
    for (auto instr : *bb->get_instrs())
    {
        assert(is_live(instr));
    }
    for (auto phi : *bb->get_phinodes())
    {
        assert(is_live(phi));
    }
    bb->drop();
}
void DCE::print()
{
    FILE *fp = freopen("doc/draw_source/DCE.txt", "w+", stdout);
    for (auto bb : *function->get_blocks())
    {
        for (auto instr : *bb->get_instrs())
        {
            if (is_live(instr))
                instr->print();
            // for (auto edge : *instr->get_user_list())
            // {
            //     std::cout << instr->get_ID() << ',' << is_live(instr) << "->"
            //               << edge->get_user()->get_ID() << ',' << is_live((Instrution *)edge->get_user()) << std::endl;
            // }
        }
        for (auto phi : *bb->get_phinodes())
        {
            if (is_live(phi))
                phi->print();
        }
    }
    fclose(fp);
}