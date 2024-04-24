#include "ir/Function.hpp"
#include "ir/Instrution.hpp"
#include <ir_opt/SCCP.hpp>

void SCCP::run()
{
    init();
    while (!cfg_worklist.empty() && !ssa_worklist.empty())
    {
        while (!cfg_worklist.empty())
        {
            Edge *edge = cfg_worklist.front();
            cfg_worklist.pop();

            // edge first time
            assert(executed_edge_map.find(edge) != executed_edge_map.end());
            if (executed_edge_map[edge] == true)
                continue;
            executed_edge_map[edge] = true;

            BasicBlock *cur_bb = (BasicBlock *)edge->get_user();
            for (auto phi : *cur_bb->get_phinodes())
                visit_phi(phi);

            // visit exp when block havent been visit
            assert(executed_block_map.find(cur_bb) != executed_block_map.end());
            if (executed_block_map[cur_bb] == true)
                continue;
            executed_block_map[cur_bb] = true;

            printf("first enter this block\n");
            for (auto instr : *cur_bb->get_instrutions())
                visit_exp(instr);

            // only have one out-edge
            // jmp
            if (cur_bb->get_user_list()->size() == 1)
            {
                auto _userlist = cur_bb->get_user_list();
                cfg_worklist.push((*_userlist)[0]);
            }
        }
        while (!ssa_worklist.empty())
        {
            Edge *ssaedge = ssa_worklist.front();
            ssa_worklist.pop();

            Instrution *instr = (Instrution *)ssaedge->get_user();
            if (instr->isPHINode() == true) // for ssa destination is phi
            {
                visit_phi(instr);
            }
            else // for destination is exp(instruction)
            {
                // block reachable
                if (executed_block_map[instr->get_parent()] == false)
                    continue;
                visit_exp(instr);
            }
        }
    }
}

void SCCP::init()
{
    // entry bb cfg_worklist
    assert(function->get_entryBB());
    Edge *fakeedge = new Edge(nullptr, function->get_entryBB());
    cfg_worklist.push(fakeedge);

    //  executed_block_edge_map init
    for (BasicBlock *bb : *function->get_basic_blocks())
    {
        auto jmporbranch = bb->get_last_instrution();
        assert(jmporbranch->isBranch() || jmporbranch->isJmp() || jmporbranch->isReturn());
        // set BB out-edge
        for (auto edge : *bb->get_user_list())
        {
            executed_edge_map.emplace(edge, false);
        }
    }
    // executed_block_map init
    for (auto bb : *function->get_basic_blocks())
    {
        executed_block_map.emplace(bb, false);
    }
    // param
    for (auto value : *function->get_params())
        latticemap.emplace(value, Lattice(Lattice::Lat::NAC));
    // other Value
    for (auto value : *function->get_Values())
        if (latticemap.find(value) == latticemap.end())
            latticemap.emplace(value, Lattice());
}

void SCCP::visit_phi(PHINode *)
{
}
void SCCP::visit_exp(Instrution *instrution)
{
    bool has_change = false;
    if (instrution->isBinary())
    {
    }
    else if (instrution->isUnary())
    {
    }
    else if (instrution->isGEP())
    {
    }
    else if (instrution->isCall())
    {
    }
    else if (instrution->isCmp())
    {
    }
    else if (instrution->isJmp())
    { // do nothing
    }
    else if (instrution->isLoad())
    {
    }
    else if (instrution->isReturn())
    { // do nothing
    }
    else if (instrution->isStore())
    { // do nothing
    }
    else if (instrution->isBranch())
    {
        // branch
        assert(latticemap.find(instrution) != latticemap.end());
        if (latticemap[instrution].lattice == Lattice::Lat::NAC)
        {
            for (auto nextbb : *instrution->get_parent()->get_user_list())
                cfg_worklist.push(nextbb);
        }
        else if (latticemap[instrution].lattice == Lattice::Lat::CONST)
        {
            if (latticemap[instrution].const_value) // true
                cfg_worklist.push((*instrution->get_parent()->get_user_list())[0]);
            else // false
                cfg_worklist.push((*instrution->get_parent()->get_user_list())[1]);
        }
    }
    else
    {
        assert(0);
    }
}