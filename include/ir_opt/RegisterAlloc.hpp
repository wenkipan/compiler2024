#pragma once
#include <unordered_set>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Manager.hpp"
#include "../../include/util/hash_pair.hpp"

class RegisterAlloc
{
    int K;
    LivenessAnalysis LA;
    std::unordered_set<std::pair<int, int>, pair_hash> adjSet;
    int vregNum;
    std::unordered_map<int, int> color;
    std::unordered_set<int> precolored;
    std::unordered_set<int> coloredNodes;
    std::unordered_map<int, std::unordered_set<int>> adjList;
    std::unordered_map<int, int> degree;
    std::unordered_map<int, int> alias;
    std::unordered_map<int, std::unordered_set<std::pair<int, int>, pair_hash>> moveList;
    std::unordered_set<std::pair<int, int>, pair_hash> frozenMoves;
    std::unordered_set<std::pair<int, int>, pair_hash> worklistMoves;
    std::unordered_set<std::pair<int, int>, pair_hash> activeMoves;
    std::unordered_set<std::pair<int, int>, pair_hash> coalescedMoves;
    std::unordered_set<std::pair<int, int>, pair_hash> constrainedMoves;
    std::unordered_set<int> initial;
    std::unordered_set<int> spillWorklist;
    std::unordered_set<int> freezeWorklist;
    std::unordered_set<int> simplifyWorklist;
    std::unordered_set<int> coalescedNodes;
    std::unordered_set<int> spilledNodes;
    std::vector<int> selectStack;
    void build(Function *p_func);
    void AddEdge(int u, int v);
    void MakeWorklist();
    bool MoveRelated(int n);
    std::unordered_set<std::pair<int, int>, pair_hash> NodeMoves(int n);
    std::unordered_set<int> Adjacent(int n);
    void Simplify();
    void DecrementDegree(int m);
    void EnableMoves(std::unordered_set<int> nodes);
    void Coalesce();
    void AddWorkList(int u);
    bool OK(int t, int r);
    bool Conservative(std::unordered_set<int> nodes);
    int GetAlias(int n);
    void Combine(int u, int v);
    void Freeze();
    void FreezeMoves(int u);
    void SelectSpill();
    void AssignColors();
    void RewriteProgram(Function *p_func);
    bool any_t_in_adjv_ok_tu(int u, int v);

public:
    void run(Function *p_func);
};