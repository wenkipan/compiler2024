#include <ir_opt/LCSSA.hpp>
#include <algorithm>

static inline void dealVal(Value *val, Loop *loop, DomTree &domtree)
{
    auto userList = val->get_user_list();
    auto exitslist = loop->get_exits();
    std::unordered_map<BasicBlock *, PHINode *> Phimap;
    std::set<Value *> vis;
    std::vector<Edge *> Del;
    for (auto it = userList->begin(); it != userList->end();)
    {
        Instrution *_user = (Instrution *)(*it)->get_user();
        BasicBlock *_BB = _user->get_parent();
        if (_user->isPHINode())
        {
            if (vis.find(_user) != vis.end())
            {
                ++it;
                continue;
            }
            _BB = ((PHINode *)_user)->get_edge_income_block(*it);
        }
        if (loop->is_BBinLoop(_BB))
        {
            ++it;
            continue;
        }
        bool flag = false;
        for (BasicBlock *_exit : *exitslist)
            if (domtree.is_dom(_exit, _BB))
            {
                auto Find = Phimap.find(_exit);
                if (Find == Phimap.end())
                {
                    PHINode *Phi = new PHINode(_exit, val->get_type(), true);
                    vis.insert(Phi);
                    for (auto Edge : *_exit->get_value_list())
                    {
                        BasicBlock *prev = (BasicBlock *)Edge->get_val();
                        Phi->addIncoming(val, prev);
                    }
                    Phimap.insert({_exit, Phi});
                    _exit->Insert_Phi(Phi);
                    printf("\nb%d\n", _exit->get_ID());
                    Phi->print();
                    _user->print();
                    flag = true;
                    it = userList->begin();
                    break;
                }
                (*it)->set_val(Find->second);
                Del.emplace_back(*it);
                flag = true;
                ++it;
                break;
            }
        assert(flag);
    }
    for (auto it : Del)
        userList->erase(std::remove(userList->begin(), userList->end(), it), userList->end());
}

void LCSSA::loopdealer(Loop *loop, DomTree &domtree)
{
    for (Loop *son : *loop->get_lpsons())
        loopdealer(son, domtree);
    for (BasicBlock *BB : *loop->get_nwBBs())
    {
        for (PHINode *_Phi : *BB->get_phinodes())
            dealVal(_Phi, loop, domtree);
        std::vector<Instrution *> *instrs = BB->get_instrutions();
        for (Instrution *p_instr : *instrs)
            dealVal(p_instr, loop, domtree);
    }
}

void LCSSA::run()
{
    Loop *loop = p_LA->get_LoopInfo()->find(p_func)->second;
    DomTree domtree(p_func);
    domtree.Run();
    puts("\nLCSSA\n");
    for (Loop *_loop : *loop->get_lpsons())
        loopdealer(_loop, domtree);
}