#include <ir_opt/DomTree.hpp>

std::unordered_map<Function*, DomTree*>*Func_map_Dom = new std::unordered_map<Function*, DomTree*>;

DomTreeNode::DomTreeNode(BasicBlock *BB)
{
    parent = BB;
    doms = new std::vector<DomTreeNode *>();
    idoms = new std::vector<DomTreeNode *>();
    Sdom = nullptr;
    Idom = nullptr;
    fa = nullptr;
    uni = nullptr;
    mn = nullptr;
    sdom_graph = new std::vector<DomTreeNode *>();
}

DomTreeNode::~DomTreeNode()
{
    delete doms;
    delete idoms;
    delete sdom_graph;
}

DomTree::DomTree(Function *Func)
{
    parent = Func;
    BB_map_Dom = new std::unordered_map<BasicBlock *, DomTreeNode *>();
    DomTreeNodes = new std::vector<DomTreeNode *>();
    std::vector<BasicBlock *> *BBs = Func->get_blocks();
    for (BasicBlock *BB : *BBs)
    {   
        DomTreeNodes->push_back(new DomTreeNode(BB));
        BB_map_Dom->insert(std::make_pair(BB, DomTreeNodes->back()));
    }
    order = new std::vector<DomTreeNode *>();
}
DomTree::~DomTree()
{
    delete BB_map_Dom;
    for (auto it : *DomTreeNodes)
        delete it;
    delete DomTreeNodes;
    delete order;
}

DomTreeNode* DomTree::get_DomTreeNode(BasicBlock *BB)
{
    return BB_map_Dom->find(BB)->second;
}

void DomTree::Run()
{
    MakeDom();
}

void DomTree::MakeDomInit()
{
    DomFsBlock.clear();
    for (DomTreeNode *BB : *DomTreeNodes)
    {
        BB->dfn = 0;
        BB->sdom_graph->clear();
    }
    order->resize(1);
    timer = 0;
}

void DomTree::dfs(DomTreeNode *u)
{
    u->dfn = ++timer;
    order->push_back(u);
    BasicBlock *BB = u->parent;
    for (Edge *edge : *(BB->get_user_list()))
    {
        DomTreeNode *v = BB_map_Dom->find((BasicBlock *)edge->get_user())->second;
        if (!v->dfn)
        {
            v->fa = u;
            dfs(v);
        }
    }
}

DomTreeNode* DomTree::Query_uni(DomTreeNode *u)
{
    if (u == u->uni)
        return u;
    DomTreeNode * res = Query_uni(u->uni);
    if (u->uni->mn->Sdom->dfn < u->mn->Sdom->dfn)
        u->mn = u->uni->mn;
    return u->uni = res;
}

void DomTree::MakeDom()
{
    BasicBlock* enter = parent->get_entryBB();
    MakeDomInit();
    dfs(BB_map_Dom->find(enter)->second);
    for (DomTreeNode *BB : *DomTreeNodes)
        BB->uni = BB->mn = BB->Sdom = BB;
    for (int i = timer; i >= 2; --i)
    {
        DomTreeNode *u = (*order)[i];
        for (Edge *edge : *(u->parent->get_value_list()))
        {
            DomTreeNode *v = BB_map_Dom->find((BasicBlock*)edge->get_val())->second;
            if (!v->dfn)
                continue;
            Query_uni(v);
            if (v->mn->Sdom->dfn < u->Sdom->dfn)
                u->Sdom = v->mn->Sdom;
        }
        u->uni = u->fa;
        u->Sdom->sdom_graph->push_back(u);
        for (DomTreeNode *v : *((u = u->fa)->sdom_graph))
        {
            Query_uni(v);
            v->Idom = ((u == v->mn->Sdom) ? u : v->mn);
        }
        u->sdom_graph->clear();
    }
    for (int i = 2; i <= timer; ++i)
    {
        DomTreeNode *u = (*order)[i];
        if (u->Idom != u->Sdom)
            u->Idom = u->Idom->Idom;
    }
    for (int i = timer; i >= 2; --i)
    {
        DomTreeNode *u = (*order)[i];
        DomTreeNode *f = u->Idom;
        u->doms->push_back(u);
        f->idoms->push_back(u);
        for (DomTreeNode *v : *(u->doms))
            f->doms->push_back(v);
    }
}

BasicBlock* DomTree::get_idom(BasicBlock *BB)
{
    if (BB_map_Dom->find(BB)->second->Idom == nullptr)
        return nullptr;
    return BB_map_Dom->find(BB)->second->Idom->parent;
}

bool DomTree::is_dom(BasicBlock *A, BasicBlock *B)
{
    for (DomTreeNode *tmp : *(get_DomTreeNode(A)->doms))
        if (tmp->parent == B)
            return true;
    return false;
}

void DomTree::get_DF()
{
    Function *Func = parent;
    for (BasicBlock *BB : *(Func->get_blocks()))
        DomFsBlock[BB] = {};
    BasicBlock *preBB, *idomBB;
    for (BasicBlock *BB : *(Func->get_blocks()))
    {
        if (!get_dfn(BB))
            continue;
        if (BB->get_value_list()->size() > 1)
        {
            idomBB = get_idom(BB);
            for (Edge *edge : *(BB->get_value_list()))
            {
                preBB = dynamic_cast<BasicBlock *>(edge->get_val());
                if (!get_dfn(preBB))
                    continue;
                while (preBB != idomBB)
                {
                    DomFsBlock[preBB].push_back(BB);
                    preBB = get_idom(preBB);
                }
            }
        }
    }
}