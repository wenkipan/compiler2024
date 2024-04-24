#pragma once

#include <ir/ir.hpp>
#include <queue>
#include <map>
class Lattice
{
public:
    enum class Lat
    {
        UNDEF, // T
        CONST, // C
        NAC    // _1_
    } lattice;
    int const_value;

    Lattice() { lattice = Lat::UNDEF; }
    Lattice(Lat lat) { lattice = lat; }
    bool set_nac_lattice();
    bool set_val_lattice();
};
// struct BBedge
// {
//     BasicBlock *value;
//     BasicBlock *user;
// };
class SCCP
{
public:
    Function *function;
    std::unordered_map<Value *, Lattice> latticemap;
    std::unordered_map<BasicBlock *, bool> executed_block_map;
    std::unordered_map<Edge *, bool> executed_edge_map;

    std::queue<Edge *> cfg_worklist;
    std::queue<Edge *> ssa_worklist;

    // SCCP();
    void run();
    void init();
    void visit_phi();
    void visit_exp();
    void add_cfg_edge_to_worklist(BasicBlock *);
    void pop_cfg_edge_from_worklist();
    void add_ssa_edge_to_worklist();
    void pop_ssa_edge_from_worklist();
};

// (1) Initialize the FlowWorkList to contain the edges exiting the start node of
// the program. The SSAWorkList is initially empty.
// Each program flow graph edge has an associated flag, the Executable-
// Flag, that controls the evaluation of PHI functions in the destination node
// of that edge. This flag is initially false for all edges.
// Each LatticeCell is initially T.
// (2) Halt execution when both worklists become empty. Execution may pro-
// ceed by processing items from either worklist.
// (3) If the item is a program flow graph edge from the FlowWorkList, then
// examine the ExecutableFlag of that edge. If the ExecutableFlag is true
// do nothing; otherwise:
//      (a) Mark the ExecutableFlag of the edge as true.
//      (b) Perform Visit-PHI for all of the PHI functions at the destination node.
//      (c) If only one of the ExecutableFlags associated with the incoming
//      program flow graph edges is true (i.e., if this is the first time this
//      node has been evaluated), then perform VisitExpression for the ex-
//      pression in this node.
//      (d) If the node only contains one outgoing flow graph edge, add that edge
//      to the FlowWorkList.
// (4) If the item is an SSA edge from the SSAWorkList and the destination of
// that edge is a PHI-function, perform Visit-PHI.
// (5) If the item is an SSA edge from the SSAWorkList and the destination of
// that edge is an expression, then examine ExecutableFlags for the pro-
// gram flow edges reaching that node. If any of them are true, perform
// VisitExpression. Otherwise do nothing.

//
// Visit-PHI is defined as follows: The LatticeCells for each operand of the
// o-function are defined on the basis of the ExecutableFlag for the correspond-
// ing program flow edge.
// executable:
// The LatticeCell has the same value as the LatticeCell at the
// definition end of the SSA edge.
// not–executable:
// The LatticeCell has the value T.

// VisitExpression is defined as follows: Evaluate the expression obtaining
// the values of the operands from the LatticeCells where they are defined and
// using the expression rules defined in Section 2.2. If this changes the value of
// the LatticeCell of the output of the expression, do the following:
// (1) If the expression is part of an assignment node, add to the SSAWorkList
// all SSA edges starting at the definition for that node.
// (2) If the expression controls a conditional branch, some outgoing flow graph
// edges must be added to the FlowWorkList. If the LatticeCell has value
// 1 , all exit edges must be added to the FlowWorkList. If the value is %’,
// only the flow graph edge executed as the result of the branch is added to
// the FlowWorkList.

// ACM ‘Mansactlons on Programming Languages and Systems, Vol 13, No 2, Aprd 1991
// Constant Propagation With Conditional Branch
