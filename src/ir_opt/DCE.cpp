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
        for (auto instr : *bb->get_instrs())
        {
            // side-effect and terminator
            switch (instr->get_Instrtype())
            {
            case InstrutionEnum::Store:
            case InstrutionEnum::Ret:
            case InstrutionEnum::Call:
                worklist.push(instr);
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
        while (!worklist.empty())
        {
            Instrution *instr = worklist.front();
            worklist.pop();

            if (is_live(instr)) // already checked
                continue;
            live_instr.emplace(instr);
            printf("marklive:");
            instr->print();
            printf("\n");
            markblocklive(instr->get_parent());

            if (instr == instr->get_parent()->get_last_instrution())
            { // succblocks
                for (auto edge : *instr->get_parent()->get_user_list())
                    markblocklive((BasicBlock *)edge->get_user());
            }

            for (auto edge : *instr->get_value_list())
            {
                if (is_a<Instrution>(edge->get_val()))
                {
                    worklist.push((Instrution *)edge->get_val());
                    printf("push:");
                    edge->get_val()->print();
                    printf("\n");
                }
            }
            if (is_a<PHINode>(instr))
            { // CFLive
                markphilive((PHINode *)instr);
            }
        }
        // After data flow liveness has been identified, examine which branch
        // decisions are required to determine live instructions are executed.
        mark_live_from_CDG();

    } while (!worklist.empty());

    // print();
    for (auto BB : *function->get_blocks())
    {
        BB->print_ID();
        printf("CDGPREDS");
        BasicBlock *reverseBB = pdt->blockmap.find(BB)->second;
        auto CDGpreds = pdt->pdt->DomFsBlock.find(reverseBB)->second;
        for (auto bb : CDGpreds)
        {

            auto itrereverseBB = pdt->blockmapreverse.find(bb);
            if (itrereverseBB == pdt->blockmapreverse.end())
            {
                assert(bb == pdt->entry);
                printf("entry");
                continue;
            }
            BasicBlock *rereverseBB = itrereverseBB->second;
            rereverseBB->print_ID();
            if (rereverseBB->get_instrs()->size())
            {
            }
        }
        printf("\n");
    }
    elimate();
}
void DCE::markphilive(PHINode *phi)
{
    if (phichecked.find(phi->get_parent()) != phichecked.end())
        return;
    phichecked.emplace(phi->get_parent());
    for (auto edge : *phi->get_parent()->get_value_list())
    {
        assert(is_a<BasicBlock>(edge->get_val()));
        BasicBlock *prebb = (BasicBlock *)edge->get_val();
        if (CFlive_block.find(prebb) == CFlive_block.end())
        {
            CFlive_block.emplace(prebb);
            newliveblocks.emplace(prebb);
        }
    }
}
void DCE::markblocklive(BasicBlock *bb)
{
    if (live_blocks.find(bb) != live_blocks.end())
        return;
    live_blocks.emplace(bb);

    if (CFlive_block.find(bb) == CFlive_block.end())
    {
        CFlive_block.emplace(bb);
        newliveblocks.emplace(bb);
    }

    worklist.push(bb->get_last_instrution());
    printf("push:");
    bb->get_last_instrution()->print();
    printf("\n");
}

void DCE::mark_live_from_CDG()
{
    // newliveblocks :: add their CDG block terminator to worklist
    for (auto BB : newliveblocks)
    {
        printf("CDG:BB");
        BB->print_ID();
        BasicBlock *reverseBB = pdt->blockmap.find(BB)->second;
        std::vector<BasicBlock *> CDGpreds = pdt->pdt->DomFsBlock.find(reverseBB)->second;
        for (auto bb : CDGpreds)
        {
            printf("CDG:predBB");
            auto itrereverseBB = pdt->blockmapreverse.find(bb);
            if (itrereverseBB == pdt->blockmapreverse.end())
            {
                assert(bb == pdt->entry);
                continue;
            }
            BasicBlock *rereverseBB = itrereverseBB->second;
            rereverseBB->print_ID();
            if (rereverseBB->get_instrs()->size())
            {
                worklist.push(rereverseBB->get_last_instrution());
                printf("pushfromCDG:");
                rereverseBB->get_last_instrution()->print();
                printf("\n");
            }
        }
    }
    newliveblocks.clear();
}
void DCE::elimate()
{
    std::vector<Instrution *> valuelist;
    std::vector<PHINode *> philist;
    std::queue<PHINode *> simplyphilist;
    // elimate dead block and its phi user
    auto vecbb = *function->get_blocks();
    for (int i = vecbb.size() - 1; i >= 0; i--)
    {
        // if (!is_live(vecbb[i]))
        // {
        //     elimate_block(vecbb[i]);
        //     continue;
        // }
        for (auto instr : *vecbb[i]->get_instrs())
        {
            if (!is_live(instr))
            {
                valuelist.push_back(instr);
            }
        }
        for (auto phi : *vecbb[i]->get_phinodes())
        {
            if (!is_live(phi))
            {
                philist.push_back(phi);
            }
            // else if (phi->get_value_list()->size() == 1)
            //     simplyphilist.push(phi);
        }
    }
    dropInstrs(valuelist);
    for (int i = philist.size() - 1; i >= 0; i--)
    {
        philist[i]->drop();
    }
    // while (!simplyphilist.empty())
    // {
    //     PHINode *p = simplyphilist.front();
    //     simplyphilist.pop();
    //     simplifyPHI(p);
    // }
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
    phi->get_user_list()->clear();
    phi->drop();
}
void DCE::elimate_block(BasicBlock *bb)
{
    printf("DELETEBBB:");
    bb->print_ID();
    printf("\n");
    for (auto instr : *bb->get_instrs())
    {
        instr->print();
        printf("\n");
        assert(is_a<Instrution>(instr));
        assert(!is_live(instr));
    }
    for (auto phi : *bb->get_phinodes())
    {
        assert(!is_live(phi));
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