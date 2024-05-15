#include <ir_opt/Loop.hpp>
#include <ir_opt/DomTree.hpp>
#include <iostream>
#include <stack>

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