#include <ir_opt/Loop.hpp>
#include <ir_opt/DomTree.hpp>
#include <iostream>
#include <stack>
#include <algorithm>

void Loop::createPrevHeader(Loop *loop)
{
    BasicBlock *header = loop->get_header();
    BasicBlock *prev = header->get_func()->block_addnewBB();
    std::set<BasicBlock *> *_enters = loop->get_enters();
    for (auto _phi : *header->get_phinodes())
    {
        std::unordered_map<BasicBlock *, Edge *> *_map = _phi->get_valueMap();
        PHINode *newPhi = new PHINode(prev, _phi->get_type()->get_type(), true);
        prev->Insert_Phi(newPhi);
        std::queue<std::pair<Edge *, BasicBlock *>> edges;
        for (auto it : *_map)
        {
            if (loop->get_BBs()->find(it.first) != loop->get_BBs()->end())
                continue;
            edges.push(std::make_pair(it.second, it.first));
        }
        while (!edges.empty())
        {
            newPhi->addIncoming(edges.front().first->get_val(), edges.front().second);
            edges.front().first->drop();
            edges.pop();
        }
        _phi->addIncoming(newPhi, prev);
    }
    std::vector<Edge *> *_edges;
    for (BasicBlock *BB : *_enters)
    {
        Instrution *p_branch = BB->get_last_instrution();
        switch (p_branch->get_Instrtype())
        {
        case InstrutionEnum::Jmp:
        case InstrutionEnum::Branch:
            _edges = header->get_value_list();
            for (auto it : *_edges)
            {
                if (it->get_val() != BB)
                    continue;
                it->reset_user(prev);
                break;
            }
            break;
        default:
            assert(0);
            break;
        }
    }
    prev->Set_jmp(header);
    loop->set_prevHead(prev);
}

bool Loop::is_simple()
{
    if (latchs->size() == 1 && exitings->size() == 1)
        return true;
    return false;
}

bool NaLoop::is_InBBs(BasicBlock *_BB)
{
    if (BBs->find(_BB) != BBs->end())
        return true;
    return false;
}

void Loop_Analysis::buildnest(Loop *nwloop, int nwlevel, std::unordered_map<BasicBlock *, Loop *> &_map)
{
    nwlevel++;
    std::set<Loop *> *sons = nwloop->get_lpsons();
    for (auto it : _map)
    {
        if (nwlevel != (*BBmap)[it.first]->get_depth())
            continue;
        if (nwlevel == 1)
            sons->insert(it.second);
        else if (nwloop->get_BBs()->find(it.first) != nwloop->get_BBs()->end())
            sons->insert(it.second);
        else
            continue;
        it.second->set_depth(nwlevel);
        it.second->set_parent(nwloop);
        std::set<BasicBlock *> *_nwBBs = it.second->get_nwBBs();
        for (BasicBlock *_BB : *it.second->get_BBs())
        {
            LoopNode *_node = (*BBmap)[_BB];
            if (_node->get_depth() == nwlevel)
            {
                assert(_node->get_loop() == nullptr);
                _node->set_loop(it.second);
                _nwBBs->insert(_BB);
            }
        }
        buildnest(it.second, nwlevel, _map);
    }
}

void _lpprint(Loop *nwloop)
{
    if (nwloop->get_header())
        printf("header b%d: ", nwloop->get_header()->get_ID());
    else
        printf("not in loop BBs: ");

    std::set<BasicBlock *> *_BBs = nwloop->get_BBs();
    for (auto _nw : *_BBs)
        printf("b%d, ", _nw->get_ID());
    if (nwloop->get_lpDepth() <= 1)
        printf("Depth %d.\n", nwloop->get_lpDepth());
    else
        printf("Depth %d. fa header b%d\n", nwloop->get_lpDepth(), nwloop->get_parent()->get_header()->get_ID());
    for (Loop *_son : (*nwloop->get_lpsons()))
        _lpprint(_son);
}

static inline void _loopBBssAdd(Loop *nwloop, BasicBlock *BB, BasicBlock *nwBB)
{
    if (BB == nwloop->get_header())
        nwloop->get_latchs()->insert(nwBB);
    else if (nwloop->get_BBs()->find(BB) == nwloop->get_BBs()->end())
        nwloop->get_exitings()->insert(nwBB), nwloop->get_exits()->insert(BB);
}

void Loop_Analysis::loop_BBsAdd(Loop *nwloop)
{

    for (Loop *_son : (*nwloop->get_lpsons()))
        loop_BBsAdd(_son);
    if (!nwloop->get_lpDepth())
        return;
    for (auto *_edge : (*nwloop->get_header()->get_value_list()))
    {
        BasicBlock *p_BB = (BasicBlock *)_edge->get_val();
        if (nwloop->get_BBs()->find(p_BB) == nwloop->get_BBs()->end())
            nwloop->get_enters()->insert(p_BB);
    }
    for (BasicBlock *_BB : (*nwloop->get_nwBBs()))
    {
        Instrution *p_branch = _BB->get_last_instrution();
        switch (p_branch->get_Instrtype())
        {
        case InstrutionEnum::Jmp:
            _loopBBssAdd(nwloop, ((Jmp *)p_branch)->get_nextBB(), _BB);
            break;
        case InstrutionEnum::Branch:
            _loopBBssAdd(nwloop, ((Branch *)p_branch)->get_trueBB(), _BB);
            _loopBBssAdd(nwloop, ((Branch *)p_branch)->get_falseBB(), _BB);
            break;
        default:
            assert(0);
            break;
        }
    }
    nwloop->createPrevHeader(nwloop);
    return;
    printf("HEAD b%d:", nwloop->get_header()->get_ID());
    printf("\nenters: ");
    for (BasicBlock *_BB : *nwloop->get_enters())
        printf("b%d, ", _BB->get_ID());
    printf("\nBBs: ");
    for (BasicBlock *_BB : *nwloop->get_BBs())
        printf("b%d, ", _BB->get_ID());
    printf("\nnwBBs: ");
    for (BasicBlock *_BB : *nwloop->get_nwBBs())
        printf("b%d, ", _BB->get_ID());
    printf("\nlatchs: ");
    for (BasicBlock *_BB : *nwloop->get_latchs())
        printf("b%d, ", _BB->get_ID());
    printf("\nexitings: ");
    for (BasicBlock *_BB : *nwloop->get_exitings())
        printf("b%d, ", _BB->get_ID());
    printf("\nexits: ");
    for (BasicBlock *_BB : *nwloop->get_exits())
        printf("b%d, ", _BB->get_ID());
    putchar('\n');
}

void Loop_Analysis::FuncAnalysis(Function *p_func)
{
    Loop *lproot = new Loop;
    DomTree domtree(p_func);
    domtree.Run();
    std::vector<BasicBlock *> *_BBs = p_func->get_blocks();
    std::unordered_map<BasicBlock *, Loop *> LoopMap;
    for (BasicBlock *_BB : *_BBs)
    {
        std::vector<Edge *> *entryEdges = _BB->get_value_list();
        DomTreeNode *DomNode = domtree.get_BBDom()->find(_BB)->second;
        for (Edge *edge : *entryEdges)
            if (DomNode->is_Dom(domtree.get_BBDom()->find((BasicBlock *)edge->get_val())->second))
            {
                std::stack<BasicBlock *> ST;
                assert(ST.empty());
                if (_BB != (BasicBlock *)edge->get_val())
                    ST.push((BasicBlock *)edge->get_val());
                NaLoop *Nloop = new NaLoop(_BB);
                Nloop->addBB((BasicBlock *)edge->get_val());
                while (!ST.empty())
                {
                    Value *_val = ST.top();
                    ST.pop();
                    std::vector<Edge *> *edges = _val->get_value_list();
                    for (auto it : *edges)
                    {
                        BasicBlock *from = (BasicBlock *)it->get_val();
                        if (!Nloop->is_InBBs(from))
                        {
                            ST.push(from);
                            Nloop->addBB(from);
                        }
                    }
                }
                if (LoopMap.find(_BB) == LoopMap.end())
                {
                    Loop *newLoop = new Loop();
                    newLoop->set_header(_BB);
                    LoopMap.insert({_BB, newLoop});
                }
                Loop *NwLoop = LoopMap.find(_BB)->second;
                std::set<BasicBlock *> *NaBBs = Nloop->get_BBs();
                std::set<BasicBlock *> *NwBBs = NwLoop->get_BBs();
                for (BasicBlock *NaBB : *NaBBs)
                {
                    if (BBmap->find(NaBB) == BBmap->end())
                    {
                        LoopNode *_new = new LoopNode();
                        BBmap->insert({NaBB, _new});
                    }
                    if (NwBBs->find(NaBB) == NwBBs->end())
                    {
                        NwBBs->insert(NaBB);
                        (*BBmap)[NaBB]->add_depth();
                    }
                }
                delete Nloop;
            }
    }
    buildnest(lproot, 0, LoopMap);
    std::vector<BasicBlock *> *_funcBBs = p_func->get_blocks();
    for (BasicBlock *_BB : *_funcBBs)
    {
        if (BBmap->find(_BB) != BBmap->end())
            continue;
        lproot->get_BBs()->insert(_BB);
    }
    LoopInfo->insert({p_func, lproot});
    std::cout << p_func->get_name() << std::endl;
    loop_BBsAdd(lproot);
    _lpprint(lproot);
}

Loop_Analysis::Loop_Analysis()
    : LoopInfo(new std::unordered_map<Function *, Loop *>),
      BBmap(new std::unordered_map<BasicBlock *, LoopNode *>)
{
}

Loop_Analysis::~Loop_Analysis()
{
    for (auto _lp : *LoopInfo)
        delete _lp.second;
    for (auto _Bs : *BBmap)
        delete _Bs.second;
    delete LoopInfo;
    delete BBmap;
}

void Loop_Analysis::PassRun(Module *p_module)
{
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncAnalysis(p_func);
    }
}

NaLoop::NaLoop(BasicBlock *_BB)
    : entryBB(_BB),
      BBs(new std::set<BasicBlock *>)
{
    BBs->insert(_BB);
}

NaLoop::~NaLoop()
{
    delete BBs;
}

Loop::Loop()
    : lpsons(new std::set<Loop *>),
      BBs(new std::set<BasicBlock *>),
      nwBBs(new std::set<BasicBlock *>),
      enters(new std::set<BasicBlock *>),
      latchs(new std::set<BasicBlock *>),
      exitings(new std::set<BasicBlock *>),
      exits(new std::set<BasicBlock *>)
{
}
Loop::~Loop()
{
    for (Loop *it : *lpsons)
        delete it;
    delete lpsons;
    delete nwBBs;
    delete BBs;
    delete enters;
    delete latchs;
    delete exitings;
    delete exits;
}