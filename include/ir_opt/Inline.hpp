#pragma once
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/IRCopy.hpp"

class CallGraphNode : public Value
{
public:
    Function *link_f;
    bool will_inline = false;
    bool recursive = 0;
    int instr_num = 0;
    int low;
    int dfn;
    int sc;
    CallGraphNode(Function *f);
};

class CallGraph
{
    CallGraphNode *root = nullptr;
    std::unordered_set<CallGraphNode *> visited;
    std::unordered_set<CallGraphNode *> tarjanstack; // infact,in sysy its useless cause we dont need scc
    std::stack<CallGraphNode *> realstack;
    std::unordered_map<Edge *, Instrution *> call_edges;
    std::unordered_map<int, int> sz;
    int dfncnt = 0;
    int sc = 0;

public:
    std::unordered_map<Function *, CallGraphNode *> supportmap;

    Instrution *get_call(Edge *e)
    {
        assert(call_edges.find(e) != call_edges.end());
        return call_edges.find(e)->second;
    }
    CallGraph(Module *m);
    void tarjan(CallGraphNode *u);
    bool can_inline(CallGraphNode *n);
    bool can_inline_recursive(CallGraphNode *n);
    void print_cg();

    ~CallGraph();
};
class Inline
{
    int if_debug = 0;
    // Module *module;
    CallGraph *CG;

public:
    void run(Module *m);
    void do_inline(Function *caller, Function *callee, Function *rec = nullptr);
    void do_inline(Function *caller, Function *callee, Instrution *call_);
    std::vector<Value *> PO(Module *m);
    void PassRun(Module *m)
    {
        run(m);
    }
};
