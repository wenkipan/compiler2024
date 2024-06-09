#include "ir/BasicBlock.hpp"
#include "util/print_for_py.hpp"
#include <ir_opt/GCM.hpp>
#include <util/RPO.hpp>
static inline int getnestdepth(BasicBlock *b, Loop_Analysis *tree)
{
    if (tree->get_BBmap()->find(b) == tree->get_BBmap()->end())
        return 0;
    return tree->get_BBmap()->find(b)->second->get_depth();
}

void GCM::init_visit_with_pinned()
{
    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
            if (ispinned(instr))
                visited.emplace(instr);
        for (auto instr : *BB->get_phinodes())
            visited.emplace(instr);
    }
}
void GCM::init(Function *func)
{
    f = func;
    root = f->get_entryBB();
    // nesttree
    nesttree = new Loop_Analysis;
    nesttree->FuncAnalysis(f);
    // domtree
    domtree = new DomTree(f);
    domtree->MakeDom();
    if (debug)
    {
        for (auto BB : *f->get_blocks())
        {
            printf("domdepthforbb");
            BB->print_ID();
            printf(" %d\n", domtree->get_deep(BB));
        }
        for (auto BB : *f->get_blocks())
        {
            printf("nestdepthforbb");
            BB->print_ID();
            printf(" %d\n", getnestdepth(BB, nesttree));
        }
        // f->print();
        //  print_for_py(f);
    }

    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
            scheduleBB.emplace(instr, BB);
        for (auto phi : *BB->get_phinodes())
            scheduleBB.emplace(phi, BB);
    }
}
void GCM::run(Function *func)
{
    init(func);
    if (debug)
        printf("EARLY\n");
    // schedule_early
    init_visit_with_pinned();
    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
            if (ispinned(instr))
                for (auto valedge : *instr->get_value_list())
                {
                    if (is_a<Instrution>(valedge->get_val()))
                        schedule_early((Instrution *)valedge->get_val());
                }
            else
                schedule_early(instr);
        for (auto phi : *BB->get_phinodes())
            for (auto valedge : *phi->get_value_list())
            {
                if (is_a<Instrution>(valedge->get_val()))
                    schedule_early((Instrution *)valedge->get_val());
            }
    }

    visited.clear();
    if (debug)
        printf("LATE\n");
    // schedule_late
    init_visit_with_pinned();
    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
            if (ispinned(instr))
                for (auto useredge : *instr->get_user_list())
                {
                    assert(is_a<Instrution>(useredge->get_user()));
                    schedule_late((Instrution *)useredge->get_user());
                }
            else
                schedule_late(instr);
        for (auto phi : *BB->get_phinodes())
            for (auto useredge : *phi->get_user_list())
            {
                assert(is_a<Instrution>(useredge->get_user()));
                schedule_late((Instrution *)useredge->get_user());
            }
    }

    move_instr_to_best();
    maintain_branch_cond();
}
bool GCM::ispinned(Instrution *instr)
{
    if (instr->isPHINode() || instr->isReturn() || instr->isBranch() || instr->isJmp() || instr->isAlloca())
        return true;
    if (instr->isLoad() || instr->isStore()) // TODO need memery info
        return true;
    if (instr->isCall()) // need call graph???
        return true;
    if (instr->isCmp())
        if (((Cmp *)instr)->isCond())
            return true;
    return false;
}
void GCM::set_scheduleBB(Instrution *i, BasicBlock *bb)
{
    assert(scheduleBB.find(i) != scheduleBB.end());
    scheduleBB.find(i)->second = bb;
}
BasicBlock *GCM::get_scheduleBB(Instrution *i)
{
    assert(scheduleBB.find(i) != scheduleBB.end());
    return scheduleBB.find(i)->second;
}
void GCM::schedule_early(Instrution *instr)
{
    if (visited.find(instr) != visited.end())
        return;
    visited.emplace(instr);
    if (debug)
    {
        printf("firstmove:");
        instr->print();
    }
    // i.block=root
    set_scheduleBB(instr, root);
    for (auto edge : *instr->get_value_list())
    {
        if (!is_a<Instrution>(edge->get_val()))
            continue;
        Instrution *x = (Instrution *)edge->get_val();
        schedule_early(x);
        if (debug)
        {
            printf("domdepthi:%d    ", domtree->get_deep(get_scheduleBB(instr)));
            instr->print();
            printf("domdepthx:%d    ", domtree->get_deep(get_scheduleBB(x)));
            x->print();
        }
        // if(i. block .dom_depth<x. block .dom_depth)
        if (domtree->get_deep(get_scheduleBB(instr)) < domtree->get_deep(get_scheduleBB(x)))
        {
            if (debug)
            {
                printf("earlymove:");
                instr->print();
            }
            // i. block:=x. block
            set_scheduleBB(instr, get_scheduleBB(x));
        }
    }
}

void GCM::schedule_late(Instrution *instr)
{
    if (visited.find(instr) != visited.end())
        return;
    if (debug)
    {
        printf("222222");
        instr->print();
    }
    visited.emplace(instr);
    BasicBlock *lca = nullptr;
    for (auto edge : *instr->get_user_list())
    {
        assert(is_a<Instrution>(edge->get_user()));
        Instrution *y = (Instrution *)edge->get_user();
        schedule_late(y);
        BasicBlock *use = get_scheduleBB(y);
        if (is_a<PHINode>(y))
            use = ((PHINode *)y)->get_edge_income_block(edge);
        lca = find_LCA(lca, use);
    }
    assert(lca);
    if (debug)
    {
        printf("LCA:BB");
        lca->print_ID();
        printf("\n");
    }
    BasicBlock *best = lca;
    if (debug)
    {
        printf("schedule:BB");
        get_scheduleBB(instr)->print_ID();
        printf("\n");
    }
    while (lca != get_scheduleBB(instr))
    {
        if (getnestdepth(lca, nesttree) < getnestdepth(best, nesttree))
            best = lca;
        if (debug)
        {
            printf("lca:BB");
            lca->print_ID();
            printf("\n");
        }
        lca = domtree->get_idom(lca);
    }
    if (debug)
    {
        printf("Best:BB");
        best->print_ID();
        printf("\n");
        printf("latemove:");
        instr->print();
    }
    set_scheduleBB(instr, best);
}
BasicBlock *GCM::find_LCA(BasicBlock *a, BasicBlock *b)
{
    if (a == nullptr)
        return b;
    while (domtree->get_deep(a) > domtree->get_deep(b))
    {
        a = domtree->get_idom(a);
        assert(a);
    }
    while (domtree->get_deep(b) > domtree->get_deep(a))
    {
        b = domtree->get_idom(b);
        assert(b);
    }
    while (a != b)
    {
        a = domtree->get_idom(a);
        b = domtree->get_idom(b);
        assert(a && b);
    }
    return a;
}
void GCM::schedule_to_block(Instrution *instr, BasicBlock *b)
{
    if (b == instr->get_parent())
        return;
    instr->insertInstr(b, b->get_instrs()->size() - 1);
}
void GCM::move_instr_to_best()
{
    std::unordered_map<BasicBlock *, int> insertpos;

    std::queue<Instrution *> work;
    for (auto BB : RPO(f))
        for (auto instr : *BB->get_instrs())
            if (get_scheduleBB(instr) != BB)
                work.emplace(instr);

    while (!work.empty())
    {
        Instrution *i = work.front();
        work.pop();
        BasicBlock *best = get_scheduleBB(i);
        if (domtree->get_deep(i->get_parent()) < domtree->get_deep(best))
        {
            if (insertpos.find(best) == insertpos.end())
                insertpos.emplace(best, 0);
            i->insertInstr(best, insertpos[best]);
            (insertpos[best])++;
        }
        else
        {
            if (best->get_last_instrution()->isBranch())
                i->insertInstr(best, best->get_instrs()->size() - 2);
            else
                i->insertInstr(best, best->get_instrs()->size() - 1);
        }
    }
}
void GCM::maintain_branch_cond()
{
    for (auto BB : *f->get_blocks())
    {
        if (BB->get_last_instrution()->isBranch())
        {
            Value *cond = ((Branch *)BB->get_last_instrution())->get_cond();
            auto instrs = *BB->get_instrs();
            // BB->print();
            if (cond != instrs[instrs.size() - 2])
            {
                assert(0);
                assert(is_a<Instrution>(cond));
                ((Instrution *)cond)->insertInstr(BB, instrs.size() - 1);
            }
        }
    }
}
GCM::~GCM()
{
    delete domtree;
    delete nesttree;
}