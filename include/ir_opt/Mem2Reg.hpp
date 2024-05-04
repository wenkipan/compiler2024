#pragma once

#include <ir/ir.hpp>
#include <ir_opt/DomTree.hpp>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include <stack>

class Mem2Reg
{
    std::vector<Alloca*> Allocas;
    std::unordered_map<Alloca*, std::set<BasicBlock*> > DefBlocks;
    std::unordered_map<Alloca*, std::set<BasicBlock*> > UseBlocks;
    std::unordered_map<BasicBlock*, std::unordered_map<PHINode*, Alloca*> > PhiMap;
    std::unordered_map<Alloca*, Value*> ValueMap;
    Store *OnlyStore;
    DomTree *DT;
public:
    ~Mem2Reg();
    int get_Instr_id(BasicBlock *BB, Instrution *instr);
    bool rewriteSingleStoreAlloca(Alloca *alloc);
    void removeFromAllocaList(unsigned& AllocaId);
    bool isAggregateType(Type* type);
    bool isPromote(Alloca* alloc);
    void collectPromotedAllocas(Function *Func);
    void clear();
    void analysisAlloca(Alloca *alloc);
    void work(Function *Func);
    bool run(Function *Func);
};