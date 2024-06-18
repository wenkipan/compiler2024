#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include "ir/Value.hpp"
#include <iostream>
#include <ir_opt/Inline.hpp>
#include <ostream>
#include <unordered_set>
#include <algorithm>
#include <vector>

//
CallGraphNode::CallGraphNode(Function *f)
{
    link_f = f;
    for (auto BB : *f->get_blocks())
    {
        instr_num += BB->get_instrs()->size();
        // phi is not need because assembly dont gen phi
    }
}
bool CallGraph::can_inline(CallGraphNode *n)
{
    if (n->instr_num > 500) // magic number dont ask me ask hujj-nb
        return false;
    if (n->recursive)
        return false;
    if (sz[n->sc] > 1)
        return false;
    if (n->link_f->get_name() == "main")
        return false;
    return true;
}

CallGraph::CallGraph(Module *m)
{
    for (auto func : *m->get_funcs())
    {
        if (func->get_isExternal())
            continue;
        CallGraphNode *cn = new CallGraphNode(func);
        if (root == nullptr && func->get_name() == "main")
            root = cn;
        supportmap.emplace(func, cn);
    }
    for (auto func : *m->get_funcs())
    {
        if (func->get_isExternal())
            continue;
        for (auto BB : *func->get_blocks())
            for (auto instr : *BB->get_instrs())
                if (instr->isCall())
                {
                    assert(is_a<Function>(instr->get_value_list()->at(0)->get_val()));
                    Function *callee = (Function *)instr->get_value_list()->at(0)->get_val();
                    if (callee->get_isExternal())
                        continue;
                    if (callee == func)
                        supportmap[func]->recursive = 1;
                    new Edge(supportmap[callee], supportmap[func]);
                }
    }
    assert(root);
    tarjan(root);
}
void CallGraph::tarjan(CallGraphNode *u)
{
    visited.emplace(u);
    u->low = u->dfn = ++dfncnt;
    tarjanstack.emplace(u);
    realstack.push(u);
    for (auto useredge : *u->get_user_list())
    {
        CallGraphNode *v = (CallGraphNode *)useredge->get_user();
        if (visited.find(v) == visited.end())
        {
            tarjan(v);
            u->low = std::min(u->low, v->low);
        }
        else if (tarjanstack.find(v) != tarjanstack.end())
        {
            u->low = std::min(u->low, v->dfn);
        }
    }
    if (u->dfn == u->low)
    {
        ++sc;
        while (realstack.top() != u)
        {
            realstack.top()->sc = sc;
            sz[sc]++;
            tarjanstack.erase(realstack.top());
            realstack.pop();
        }
        assert(realstack.top() == u);
        realstack.top()->sc = sc;
        sz[sc]++;
        tarjanstack.erase(realstack.top());
        realstack.pop();
    }
}
void CallGraph::print_cg()
{
    for (auto kv : supportmap)
    {
        for (auto useredge : *kv.second->get_user_list())
        {
            Function *callee = (Function *)((CallGraphNode *)useredge->get_user())->link_f;
            std::cout << kv.first->get_name() << "," << kv.second->sc
                      << "->" << callee->get_name() << "," << supportmap[callee]->sc << std::endl;
        }
    }
}
CallGraph::~CallGraph()
{
    for (auto kv : supportmap)
    {
        for (auto useredge : *kv.second->get_user_list())
            delete useredge;
        delete kv.second;
    }
}

static void inline visit(Value *v, std::set<Value *> &visited, std::vector<Value *> &order)
{
    if (visited.find(v) != visited.end())
        return;

    visited.emplace(v);
    for (auto useredge : *v->get_user_list())
    {
        visit(useredge->get_user(), visited, order);
    }
    order.push_back(v);
}
std::vector<Value *> Inline::PO(Module *m)
{
    std::vector<Value *> order;
    std::set<Value *> visited;
    for (auto *func : *m->get_funcs())
    {
        if (func->get_isExternal())
            continue;
        if (func->get_name() == "main")
        {
            visit(CG->supportmap[func], visited, order);
        }
    }
    // std::reverse(order.begin(), order.end());
    return order;
}

void Inline::run(Module *m)
{
    CG = new CallGraph(m);
    if (if_debug)
        CG->print_cg();
    if (if_debug)
        for (auto func : PO(m))
        {
            std::cout << ((CallGraphNode *)func)->link_f->get_name() << std::endl;
        }
    // for (auto func : *m->get_funcs())
    // {
    //     if (func->get_isExternal())
    //         continue;
    //     if (CG->can_inline(CG->supportmap[func]))
    //     {
    //         if (if_debug)
    //             std::cout << "inlining" << func->get_name() << std::endl;
    //         for (auto calleredge : *CG->supportmap[func]->get_value_list())
    //         {
    //             Function *caller = (Function *)((CallGraphNode *)calleredge->get_val())->link_f;
    //             do_inline(caller, func);
    //             // tmp
    //         }
    //     }
    // }
    std::set<Function *> delelist;
    for (auto v : PO(m))
    {
        CallGraphNode *cgnode = (CallGraphNode *)v;
        if (CG->can_inline(cgnode)) // callee
        {
            if (if_debug)
                std::cout << "inlining" << cgnode->link_f->get_name() << std::endl;
            for (auto calleredge : *cgnode->get_value_list())
            {
                Function *caller = (Function *)((CallGraphNode *)calleredge->get_val())->link_f;
                do_inline(caller, cgnode->link_f);
                // tmp
            }
            delelist.emplace(cgnode->link_f);
        }
    }
    for (auto f : delelist)
    {
        for (auto it = m->get_funcs()->begin(); it != m->get_funcs()->end(); it++)
        {
            if (*it == f)
            {
                m->get_funcs()->erase(it);
                break;
            }
        }
        delete f;
    }
    delete CG;
}
void Inline::do_inline(Function *caller, Function *callee)
{
    // find callee and replace
    for (auto BB : *caller->get_blocks())
        for (auto instr : *BB->get_instrs())
            if (instr->isCall() && instr->get_value_list()->at(0)->get_val() == callee)
            {
                // all the instr below is actually a call

                // spilt BB
                BasicBlock *halfBB = new BasicBlock(caller);
                caller->block_pushBack(halfBB);
                if (BB == caller->get_retBB())
                    caller->set_retBB(halfBB);

                // move follow instrs
                auto instrs = BB->get_instrs();
                for (auto it = find(instrs->begin(), instrs->end(), instr); it != instrs->end();) // careful
                {
                    (*it)->insertInstr(halfBB, halfBB->get_instrs()->size());
                }
                // move succedges
                for (auto succedge : *BB->get_user_list())
                {
                    succedge->set_val(halfBB);
                    BasicBlock *succ = (BasicBlock *)succedge->get_user();
                    for (auto phi : *succ->get_phinodes())
                    {
                        for (auto kv : *phi->get_valueMap())
                        {
                            if (kv.first == BB)
                            {
                                phi->get_valueMap()->emplace(halfBB, kv.second);
                                phi->get_valueMap()->erase(BB);
                                break;
                            }
                        }
                    }
                }
                BB->get_user_list()->clear();
                // gen func
                IRCopy cp;
                Function *inlining = cp.copy_func(callee);

                // create a jmp to gen func entry
                new Jmp(inlining->get_entryBB(), BB);

                // set param to call valuelist,change edge
                auto itparam = inlining->get_params()->begin();
                for (auto itVal = instr->get_value_list()->begin() + 1; itVal != instr->get_value_list()->end(); itVal++, itparam++)
                {
                    for (auto useredge : *(*itparam)->get_user_list())
                        useredge->set_val((*itVal)->get_val());
                    (*itparam)->get_user_list()->clear();
                }
                assert(itparam == inlining->get_params()->end());

                //  change  call to ret val :find ret and change call s user to its val(0) ,drop ret
                Instrution *reti = inlining->get_retBB()->get_last_instrution();
                assert(is_a<Ret>(reti));
                if (reti->get_value_list()->size() == 1)
                {
                    Value *retval = reti->get_value_list()->at(0)->get_val();
                    reti->get_value_list()->at(0)->drop();
                    instr->replaceAllUses(retval);
                }
                else
                {
                    assert(reti->get_value_list()->size() == 0);
                    assert(reti->get_type()->get_type() == TypeEnum::Void);
                }
                instr->drop();
                reti->get_value_list()->clear();
                reti->drop();

                // create a jmp to halfBB
                new Jmp(halfBB, inlining->get_retBB());

                // promote alloc to func b0
                //(for local variables, promote is safe, think about diff between define and declare)
                for (auto newBBs : *inlining->get_blocks())
                    for (auto instr : *newBBs->get_instrs())
                        if (instr->isAlloca())
                            instr->insertInstr(caller->get_entryBB(), 0);

                // change newBBs to caller,clear inling_func bbs
                for (auto newBBs : *inlining->get_blocks())
                {
                    newBBs->Set_parent(caller);
                    caller->block_pushBack(newBBs);
                }
                inlining->get_blocks()->clear();

                // change value in inlining ,clear inlining vals
                for (auto val : *inlining->get_Values())
                {
                    if (!is_a<Param>(val))
                        caller->value_pushBack(val);
                }
                inlining->get_Values()->clear();
                // delete param
                for (auto it = inlining->get_params()->begin(); it != inlining->get_params()->end(); it++)
                {
                    delete (*it);
                }
                delete inlining;

                return;
            }
}