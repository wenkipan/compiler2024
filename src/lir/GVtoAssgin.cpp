#include <algorithm>

#include "../../include/lir/GVtoAssgin.hpp"
#include "../../include/util/RPO.hpp"

void GVtoA::add_useredge(GlobalVariable *g, Edge *e)
{
    if (gvused_edge.find(g) == gvused_edge.end())
    {
        std::vector<Edge *> tmp;
        tmp.push_back(e);
        gvused_edge.emplace(g, tmp);
    }
    else
        gvused_edge.find(g)->second.push_back(e);
}
// static inline int get_posoffitst(Instrution *first)
// {
//     int pos = 0;
//     for (auto i : *first->get_parent()->get_instrutions())
//     {
//         if (i == first)
//             return pos;
//         pos++;
//     }
//     return -1;
// }
static inline void erase_from_userlist(GlobalVariable *g, Edge *e)
{
    auto it = std::find(g->get_user_list()->begin(), g->get_user_list()->end(), e);
    g->get_user_list()->erase(it);
}
BasicBlock *GVtoA::find_LCA(BasicBlock *a, BasicBlock *b)
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
BasicBlock *GVtoA::find_LCA(std::vector<Edge *> users)
{
    BasicBlock *lca = nullptr;
    for (auto e : users)
    {
        assert(is_a<Instrution>(e->get_user()));
        BasicBlock *use = ((Instrution *)e->get_user())->get_parent();
        lca = find_LCA(lca, use);
    }
    return lca;
}
// static inline int find(BasicBlock *b, std::vector<Edge *> users)
// {
//     int pos = b->get_instrs()->size();
//     for (auto e : users)
//     {
//         if (((Instrution *)e->get_user())->get_parent() == b)
//         {
//             int findp = get_posoffitst((Instrution *)e->get_user());
//             if (findp != -1 && pos > findp)
//                 pos = findp;
//         }
//     }
//     return
// }
void GVtoA::run(Function *f)
{
    // collect
    for (auto b : RPO(f))
        for (auto i : *b->get_instrs())
            for (auto e : *i->get_value_list())
                if (is_a<GlobalVariable>(e->get_val()))
                {
                    add_useredge((GlobalVariable *)e->get_val(), e);
                }

    // do
    domtree = new DomTree(f);
    domtree->MakeDom();
    for (auto kv : gvused_edge)
    {
        auto userse = kv.second;
        assert(is_a<Instrution>(userse.front()->get_user()));
        BasicBlock *lca = find_LCA(kv.second);
        int pos = 0;
        if (lca != f->get_entryBB())
            pos = 0;
        else
            for (auto all : *lca->get_instrs())
            {
                if (!is_a<Alloca>(all))
                    break;
                pos++;
            }
        Instrution *newa = new Assign(InstrutionEnum::Assign, kv.first, lca, true);
        newa->insertInstr(lca, pos);
        for (auto e : userse)
        {
            e->set_val(newa);
            erase_from_userlist(kv.first, e);
        }
    }
    delete domtree;
}