#pragma once

#include "ir/Instrution.hpp"
#include "ir/Value.hpp"
#include <ir/ir.hpp>
#include <queue>
#include <map>
#include <iostream>
class Lattice
{
public:
    enum class Lat
    {
        UNDEF, // T
        CONST, // C
        NAC    // _1_
    } lat;
    union
    {
        int i;
        float f;
    };
    bool is_i;
    Lattice(Lat l) { lat = l; }
    Lattice(Lat l, int _is_i)
    {
        lat = l;
        is_i = _is_i;
    }
    Lattice(Lat _lat, int _is_i, int _i)
    {
        lat = _lat;
        assert(_is_i);
        is_i = 1;
        i = _i;
    }
    Lattice(Lat _lat, int _is_i, float f)
    {
        lat = _lat;
        assert(!_is_i);
        is_i = 0;
        i = f;
    }
    // Lattice &operator=(const Lattice &a)
    // {
    //     this->lat = a.lat;
    //     this->is_i = a.is_i;
    // }
    Lattice operator==(const Lattice &b);
    Lattice operator!=(const Lattice &b);
    Lattice operator>(const Lattice &b);
    Lattice operator>=(const Lattice &b);
    Lattice operator<(const Lattice &b);
    Lattice operator<=(const Lattice &b);

    Lattice operator+(const Lattice &b);
    Lattice operator-(const Lattice &b);
    Lattice operator*(const Lattice &b);
    Lattice operator/(const Lattice &b);
    Lattice operator%(const Lattice &b);
    Lattice operator&(const Lattice &b);
    Lattice operator|(const Lattice &b);
    Lattice operator^(const Lattice &b);
    bool exp_insersect(Lattice, Lattice, InstrutionEnum itype);
    bool phi_insersect(Lattice, Lattice &);
    Lattice constfold(Lattice lat1, Lattice lat2, InstrutionEnum itype);
    friend std::ostream &operator<<(std::ostream &output, const Lattice &D)
    {
        switch (D.lat)
        {
        case Lat::UNDEF:
            output << "UNDEF";
            break;
        case Lat::CONST:
            if (D.is_i)
                output << D.i;
            else
                output << D.f;
            break;
        case Lat::NAC:
            output << "NAC";
            break;
        default:
            break;
        }
        return output;
    }
};
class SCCP
{
    Function *function;
    std::unordered_map<Value *, Lattice> latticemap;
    std::unordered_map<BasicBlock *, bool> executed_block_map;
    std::unordered_map<Edge *, bool> executed_edge_map;
    // std::unordered_map<Value *, BasicBlock *> value2BB_map;

    std::queue<Edge *> cfg_worklist;
    std::queue<Edge *> ssa_worklist;
    Edge *fakeedge;
    BasicBlock *fakeBlock;

public:
    void run(Function *func);
    void init();
    void visit_exp(Instrution *);
    void visit_phi(PHINode *);
    bool visit_binary(Instrution *instr);
    bool visit_cmp(Instrution *instr);
    bool visit_unary(Instrution *instr);
    bool visit_GEP(Instrution *instr);
    void do_sccp();
    void print();
    void do_sccp_drop_unexecuted_blocks();
    Lattice get_lattice_from_map(Value *);
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
// PHI-function are defined on the basis of the ExecutableFlag for the correspond-
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
