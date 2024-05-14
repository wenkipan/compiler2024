#include "ir/BasicBlock.hpp"
#include "ir/Constant.hpp"
#include "ir/Edge.hpp"
#include "ir/Function.hpp"
#include "ir/Instrution.hpp"
#include <ir_opt/DCE.hpp>
#include <iostream>
#include <fstream>
void DCE::init()
{
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
    live_blocks.emplace(function->get_entryBB());
    if (function->get_entryBB()->get_last_instrution()->isJmp())
        worklist.push(function->get_entryBB()->get_last_instrution());
}
void DCE::run(Function *func)
{
    function = func;
    c = function->get_blocks()->size();
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
            if (if_debug)
            {
                printf("marklive:");
                instr->print();
            }
            markblocklive(instr->get_parent());

            if (instr == instr->get_parent()->get_last_instrution() && instr->isBranch())
            { // succblocks
                for (auto edge : *instr->get_parent()->get_user_list())
                    markblocklive((BasicBlock *)edge->get_user());
            }

            for (auto edge : *instr->get_value_list())
            {
                if (is_a<Instrution>(edge->get_val()))
                {
                    worklist.push((Instrution *)edge->get_val());
                    if (if_debug)
                    {
                        printf("push:");
                        edge->get_val()->print();
                    }
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

    if (if_debug) // print();
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
    if (bb->get_last_instrution()->isJmp())
        worklist.push(bb->get_last_instrution());
    if (if_debug)
    {
        printf("push:");
        bb->get_last_instrution()->print();
    }
}

void DCE::mark_live_from_CDG()
{
    // newliveblocks :: add their CDG block terminator to worklist
    for (auto BB : newliveblocks)
    {
        BasicBlock *reverseBB = pdt->blockmap.find(BB)->second;
        std::vector<BasicBlock *> CDGpreds = pdt->pdt->DomFsBlock.find(reverseBB)->second;
        for (auto bb : CDGpreds)
        {
            auto itrereverseBB = pdt->blockmapreverse.find(bb);
            if (itrereverseBB == pdt->blockmapreverse.end())
            {
                assert(bb == pdt->entry);
                continue;
            }
            BasicBlock *rereverseBB = itrereverseBB->second;
            if (rereverseBB->get_instrs()->size())
            {
                worklist.push(rereverseBB->get_last_instrution());
            }
        }
    }
    newliveblocks.clear();
}
void DCE::elimate()
{
    update_dead_block();
    std::vector<Instrution *> valuelist;
    std::vector<PHINode *> philist;
    std::queue<PHINode *> simplyphilist;
    // elimate dead instr
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
        }
    }
    dropInstrs(valuelist);
    for (int i = philist.size() - 1; i >= 0; i--)
    {
        philist[i]->drop();
    }
}
void DCE::caculatepostorder()
{
    search(function->get_entryBB());
    if (if_debug)
        for (auto BB : *function->get_blocks())
        {
            BB->print_ID();
            printf("      %d\n", postorder[BB]);
        }
}
void DCE::search(BasicBlock *n)
{
    postorder.emplace(n, -1);
    for (auto succedge : *n->get_user_list())
    {
        BasicBlock *succ = (BasicBlock *)succedge->get_user();
        if (postorder.find(succ) == postorder.end())
        {
            search(succ);
        }
    }
    postorder[n] = c;
    c = c - 1;
}
void DCE::update_dead_block()
{
    std::queue<Edge *> deletelist;
    int havepostorder = 0;
    std::vector<BasicBlock *> deadblocks;
    for (auto BB : *function->get_blocks())
        if (!is_live(BB->get_last_instrution()))
            deadblocks.push_back(BB);
    for (auto BB : deadblocks)
    {
        if (BB->get_instrs()->size())
        {
            if (BB->get_last_instrution()->isJmp())
            {
                live_instr.emplace(BB->get_last_instrution());
                continue;
            }
        }

        if (!havepostorder)
        {
            caculatepostorder();
            havepostorder = 1;
        }
        BasicBlock *prefersucc = nullptr;
        for (auto succedge : *BB->get_user_list())
        {
            BasicBlock *succ = (BasicBlock *)succedge->get_user();
            if (!prefersucc || postorder[prefersucc] < postorder[succ])
                prefersucc = succ;
        }
        assert(prefersucc);
        for (auto edge : *BB->get_user_list())
        {
            deletelist.push(edge);
        }
        new Jmp(prefersucc, BB);
        live_instr.emplace(BB->get_last_instrution());
    }
    drop_all_edges(deletelist);
    // function->print();
    assert(function->check_can_ret());
}

void DCE::elimate_block(BasicBlock *bb)
{
    printf("DELETEBBB:");
    bb->print_ID();
    printf("\n");
    for (auto instr : *bb->get_instrs())
    {
        instr->print();
        assert(is_a<Instrution>(instr));
        assert(!is_live(instr));
    }
    for (auto phi : *bb->get_phinodes())
    {
        assert(!is_live(phi));
    }
    bb->drop();
}
void DCE::print() // unuse
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