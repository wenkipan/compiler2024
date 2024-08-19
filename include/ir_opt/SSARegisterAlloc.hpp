#pragma once
#include <set>
#include <queue>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Manager.hpp"
#include "../../include/util/hash_pair.hpp"
#include "../../include/ir_opt/Loop.hpp"

class SSARegisterAlloc
{
    LivenessAnalysis LA;
    int vregNum;
    std::vector<Value *> Register;
    std::unordered_map<int, int> color, color_root;
    std::unordered_set<int> spilledNodes;
    std::unordered_set<Value *> spilledVals;
    std::vector<std::vector<int>> G;
    std::unordered_map<Alloca *, Param *> paraMap;
    std::unordered_map<Alloca *, GlobalVariable *> gvMap;
    std::unordered_map<Call *, std::vector<Value *>> callLiveVreg;
    std::unordered_map<Value *, Alloca *> allocMap;
    std::unordered_map<Value *, int> valueMapRegister;
    std::unordered_map<Call *, Move *> firstMoveofCall;
    std::unordered_set<Alloca *> spillAllocs;
    std::unordered_map<Alloca *, Value *> spillAllocMap;
    NestT nest_tree;
    void Spill(Function *p_func);
    void SpillBB_R(BasicBlock *bb);
    void SpillBB_Q(BasicBlock *bb);
    void SpillBB_S(BasicBlock *bb);
    void spillUserPhi(int x);
    void RewriteProgram(Function *p_func);
    void MakeGraph(Function *p_func);
    void AddEdge(int x, int y);
    void AssignColor_R(Function *p_func);
    void AssignColor_S(Function *p_func);
    void AssignColor_Q(Function *p_func);
    void AddBB(Function *p_func);
    void ReSortForPara(Function *p_func);
    void ReSortForCall(Call *call);
    void ReSortForPhi(BasicBlock *bb);
    void ReLoad(BasicBlock *bb);
    void ReplaceNullToTmp(Function *p_func);
    void AddTripleX(Function *p_func);
    void AnalysisTriple(Function *p_func);
    void DoTripleX(Function *p_func);
    int CalcLoop(int id);

public:
    std::unordered_map<Value *, std::unordered_set<Value *>> G_set;
    void run(Function *p_func);
    int getReg(Value *val);
    Param *whichPara(Alloca *alloc);
    GlobalVariable *whichGV(Alloca *alloc);
    std::vector<int> regsStillAliveAfterCall(Call *call);
    void graphBuilder(Function *p_func);
    Move *getFirstMoveofCall(Call *call)
    {
        if (firstMoveofCall.find(call) == firstMoveofCall.end())
            return nullptr;
        return firstMoveofCall[call];
    }
    bool isSpill(Alloca *alloc)
    {
        if (spillAllocs.find(alloc) != spillAllocs.end())
            return true;
        return false;
    }
};