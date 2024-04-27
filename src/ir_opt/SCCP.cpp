#include "ir/BasicBlock.hpp"
#include "ir/Constant.hpp"
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
                assert((*cur_bb->get_instrs()).back()->isJmp());
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
    do_sccp();
}

void SCCP::init()
{
    // entry bb cfg_worklist
    assert(function->get_entryBB());
    Edge *fakeedge = new Edge(nullptr, function->get_entryBB()); // TODO:delete this edge
    cfg_worklist.push(fakeedge);

    //  executed_block_edge_map init
    for (BasicBlock *bb : *function->get_blocks())
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
    for (auto bb : *function->get_blocks())
        executed_block_map.emplace(bb, false);

    // param
    for (auto value : *function->get_params())
        latticemap.emplace(value, Lattice(Lattice::Lat::NAC));

    // other Value
    for (auto value : *function->get_Values())
        if (latticemap.find(value) == latticemap.end())
        {
            assert(!is_a<Param>(value));
            if (is_a<ConstantI32>(value))
            { // what about a array??
                auto veci = ((ConstantI32 *)value)->get_i32();
                assert(veci.size() == 1);
                latticemap.emplace(value, Lattice(Lattice::Lat::CONST, 1, veci[0]));
            }
            else if (is_a<ConstantF32>(value))
            {
                auto vecf = ((ConstantF32 *)value)->get_f32();
                assert(vecf.size() == 1);
                latticemap.emplace(value, Lattice(Lattice::Lat::CONST, 0, vecf[0]));
            }
            else
                latticemap.emplace(value, Lattice(Lattice::Lat::UNDEF));
        }
}
void SCCP::do_sccp()
{
}

static inline bool is_lat_lower(Lattice::Lat latbefore, Lattice::Lat latafter)
{
    if (latbefore != latafter)
    {
        if (latbefore == Lattice::Lat::UNDEF && latafter == Lattice::Lat::CONST)
            return true;
        else if (latbefore == Lattice::Lat::CONST && latafter == Lattice::Lat::NAC)
            return true;
        else if (latbefore == Lattice::Lat::UNDEF && latafter == Lattice::Lat::NAC)
            return true;
    }
    return false;
}
void SCCP::visit_phi(Value *phi)
{
    assert(latticemap.find(phi) != latticemap.end());
    Lattice tmp = latticemap.find(phi)->second;
    if (latticemap.find(phi)->second.lat == Lattice::Lat::NAC)
        return;
    auto phi_value_list = phi->get_value_list();
    for (auto edge : *phi_value_list)
    {
        auto value = edge->get_val();
        // problem!!!
        BasicBlock *bb;
        if (is_a<Param>(value))
            bb = function->get_entryBB();
        else if (is_a<Instrution>(value))
            bb = ((Instrution *)value)->get_parent();
        // else if (is_a<Constant>(value))
        //     bb = ((Constant *)value)->get_parent();
        else
            assert(0);

        assert(executed_block_map.find(bb) != executed_block_map.end());
        if (executed_block_map[bb] == false)
            continue;

        assert(latticemap.find(value) != latticemap.end());
        Lattice lat_for_value = latticemap.find(value)->second;
        tmp.phi_insersect(tmp, lat_for_value);
    }
    if (is_lat_lower(latticemap.find(phi)->second.lat, tmp.lat))
    {
        latticemap.find(phi)->second = tmp;
        for (auto edge : *phi->get_user_list())
            ssa_worklist.push(edge);
    }
}

void SCCP::visit_exp(Instrution *instrution)
{
    assert(latticemap.find(instrution) != latticemap.end());

    bool need_add_ssa = false;
    if (instrution->isBranch())
    {
        auto cond = ((Branch *)instrution)->get_cond();
        assert(latticemap.find(cond) != latticemap.end());
        Lattice cond_lattice = latticemap.find(cond)->second;
        if (cond_lattice.lat == Lattice::Lat::NAC)
        {
            for (auto nextbb : *instrution->get_parent()->get_user_list())
                cfg_worklist.push(nextbb);
        }
        else if (cond_lattice.lat == Lattice::Lat::CONST)
        {
            assert(cond_lattice.is_i);
            if (cond_lattice.i) // true
                cfg_worklist.push((*instrution->get_parent()->get_user_list())[0]);
            else // false
                cfg_worklist.push((*instrution->get_parent()->get_user_list())[1]);
        }
    }
    else if (instrution->isBinary())
    {
        need_add_ssa = visit_binary(instrution);
    }
    else if (instrution->isUnary())
    {
        need_add_ssa = visit_unary(instrution);
    }
    else if (instrution->isGEP())
    {
        need_add_ssa = visit_GEP(instrution);
    }
    else if (instrution->isCall())
    { // must be NAC

        Lattice::Lat tmp = latticemap.find(instrution)->second.lat;
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
        if (tmp != Lattice::Lat::NAC)
            need_add_ssa = 1;
    }
    else if (instrution->isCmp())
    {
        need_add_ssa = visit_cmp(instrution);
    }
    else if (instrution->isJmp())
    { // do nothing
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
    }
    else if (instrution->isLoad())
    { // must be NAC
        Lattice::Lat tmp = latticemap.find(instrution)->second.lat;
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
        if (tmp != Lattice::Lat::NAC)
            need_add_ssa = 1;
    }
    else if (instrution->isReturn())
    { // ret dont have users so do nothing or {set to NAC}?
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
    }
    else if (instrution->isStore())
    { // do nothing
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
    }
    else
    {
        assert(0);
    }

    if (need_add_ssa)
    {
        // push edge from userlist to ssa
        auto userlist = instrution->get_user_list();
        for (auto edge : *userlist)
            ssa_worklist.push(edge);
    }
}
bool SCCP::visit_binary(Instrution *instr)
{
    auto value_list = instr->get_value_list();
    assert(value_list->size() == 2);
    assert(latticemap.find((*value_list)[0]->get_val()) != latticemap.end());
    assert(latticemap.find((*value_list)[2]->get_val()) != latticemap.end());
    auto lat1 = latticemap.find((*value_list)[0]->get_val())->second;
    auto lat2 = latticemap.find((*value_list)[1]->get_val())->second;
    return latticemap.find(instr)->second.exp_insersect(lat1, lat2, instr->get_Instrtype());
}
bool SCCP::visit_cmp(Instrution *instr)
{
    auto value_list = instr->get_value_list();
    assert(value_list->size() == 2);
    assert(latticemap.find((*value_list)[0]->get_val()) != latticemap.end());
    assert(latticemap.find((*value_list)[2]->get_val()) != latticemap.end());
    auto lat1 = latticemap.find((*value_list)[0]->get_val())->second;
    auto lat2 = latticemap.find((*value_list)[1]->get_val())->second;
    return latticemap.find(instr)->second.exp_insersect(lat1, lat2, instr->get_Instrtype());
}
bool SCCP::visit_unary(Instrution *instr)
{
    Value *src;
    Lattice tmp = latticemap.find(instr)->second;
    Lattice instr_lattice = latticemap.find(instr)->second;
    switch (instr->get_Instrtype())
    {
    case InstrutionEnum::MINUS: // problem!!!
        assert(instr->get_value_list()->size() == 1);
        src = (*instr->get_value_list())[0]->get_val();
        assert(latticemap.find(src) != latticemap.end());
        tmp = latticemap.find(src)->second;
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            if (tmp.is_i)
                tmp.i = -tmp.i;
            else
                tmp.f = -tmp.f;
            latticemap.find(instr)->second = tmp;
            return true;
        }
        return false;
    case InstrutionEnum::F2I:
        assert(instr->get_value_list()->size() == 1);
        src = (*instr->get_value_list())[0]->get_val();
        assert(latticemap.find(src) != latticemap.end());
        tmp = latticemap.find(src)->second;
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            assert(!tmp.is_i);
            tmp.is_i = 1;
            tmp.i = (float)tmp.f;
            latticemap.find(instr)->second = tmp;
            return true;
        }
        return false;
    case InstrutionEnum::I2F:
        assert(instr->get_value_list()->size() == 1);
        src = (*instr->get_value_list())[0]->get_val();
        assert(latticemap.find(src) != latticemap.end());
        tmp = latticemap.find(src)->second;
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            assert(tmp.is_i);
            tmp.is_i = 0;
            tmp.f = (int)tmp.i;
            latticemap.find(instr)->second = tmp;
            return true;
        }
        return false;
    case InstrutionEnum::AddSP: // no user so set to NAC?
        latticemap.find(instr)->second.lat = Lattice::Lat::NAC;
        return false;
    default:
        assert(0);
    }
    return false;
}
bool SCCP::visit_GEP(Instrution *instr)
{
    Lattice::Lat tmp = latticemap.find(instr)->second.lat;
    latticemap.find(instr)->second.lat = Lattice::Lat::NAC;
    if (tmp != Lattice::Lat::NAC)
        return 1;
    return 0;
}
bool Lattice::exp_insersect(Lattice lat1, Lattice lat2, InstrutionEnum itype)
{
    Lattice tmp = *this;
    if (lat1.lat == Lat::NAC || lat2.lat == Lat::NAC)
        tmp.lat = Lat::NAC;
    else if (lat1.lat == Lat::CONST && lat2.lat == Lat::CONST)
    {
        tmp.lat = Lat::CONST;
        assert(lat1.is_i == lat2.is_i);
        tmp = constfold(lat1, lat2, itype);
    }
    // does assgin to UNDEF effect?
    //  else if (lat1.lattice == Lat::CONST || lat2.lattice == Lat::CONST)
    //  {
    //      lattice = Lat::CONST;
    //      is_i = lat1.is_i;
    //  }
    else
    { // UNDEF && UNDEF
    }

    // is lattice lower ?
    if (is_lat_lower(this->lat, tmp.lat))
    {
        this->lat = tmp.lat;
        this->is_i = tmp.is_i;
        if (tmp.is_i)
            this->i = tmp.i;
        else
            this->f = tmp.f;
        return true;
    }
    return false;
}

bool Lattice::phi_insersect(Lattice lat1, Lattice &lat2)
{
    Lattice tmp = *this;
    if (lat1.lat == Lat::NAC || lat2.lat == Lat::NAC)
        tmp.lat = Lat::NAC;
    else if (lat1.lat == Lat::CONST && lat2.lat == Lat::CONST)
    {
        assert(lat1.is_i == lat2.is_i);
        if (lat1.is_i && (lat1.i == lat2.i))
        {
            tmp.lat = Lat::CONST;
            tmp.i = lat1.i;
            tmp.is_i = lat1.is_i;
        }
        else if (!lat1.is_i && (lat1.f == lat2.f))
        {
            tmp.lat = Lat::CONST;
            tmp.f = lat1.f;
            tmp.is_i = lat1.is_i;
        }
        else
            tmp.lat = Lat::NAC;
    }
    else if (lat1.lat == Lat::CONST)
        tmp.lat = Lat::CONST;
    else if (lat2.lat == Lat::CONST)
        tmp.lat = Lat::CONST;
    else
    { // UNDEF && UNDEF
    }

    // is lattice lower ?
    if (is_lat_lower(this->lat, tmp.lat))
    {
        this->lat = tmp.lat;
        this->is_i = tmp.is_i;
        if (tmp.is_i)
            this->i = tmp.i;
        else
            this->f = tmp.f;
        return true;
    }
    return false;
}

Lattice Lattice::constfold(Lattice lat1, Lattice lat2, InstrutionEnum itype)
{
    assert(lat1.lat == Lat::CONST);
    assert(lat2.lat == Lat::CONST);
    switch (itype)
    {
    case InstrutionEnum::IEQ:
        return lat1 == lat2;
    case InstrutionEnum::INEQ:
        return lat1 != lat2;
    case InstrutionEnum::IGT:
        return lat1 > lat2;
    case InstrutionEnum::IGE:
        return lat1 >= lat2;
    case InstrutionEnum::ILT:
        return lat1 < lat2;
    case InstrutionEnum::ILE:
        return lat1 <= lat2;
    case InstrutionEnum::FEQ:
        return lat1 == lat2;
    case InstrutionEnum::FNEQ:
        return lat1 != lat2;
    case InstrutionEnum::FGT:
        return lat1 > lat2;
    case InstrutionEnum::FGE:
        return lat1 >= lat2;
    case InstrutionEnum::FLT:
        return lat1 < lat2;
    case InstrutionEnum::FLE:
        return lat1 <= lat2;
    case InstrutionEnum::IADD:
        return lat1 + lat2;
    case InstrutionEnum::ISUB:
        return lat1 - lat2;
    case InstrutionEnum::IMUL:
        return lat1 * lat2;
    case InstrutionEnum::IDIV:
        return lat1 / lat2;
    case InstrutionEnum::IMOD:
        return lat1 % lat2;
    case InstrutionEnum::FADD:
        return lat1 + lat2;
    case InstrutionEnum::FSUB:
        return lat1 - lat2;
    case InstrutionEnum::FMUL:
        return lat1 * lat2;
    case InstrutionEnum::FDIV:
        return lat1 / lat2;
    case InstrutionEnum::AND:
        return lat1 & lat2;
    case InstrutionEnum::OR:
        return lat1 | lat2;
    case InstrutionEnum::XOR:
        return lat1 ^ lat2;
    default:
        assert(0);
        break;
    }
}

Lattice Lattice::operator!=(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i != b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f != b.f);
    }
    return ret;
}
Lattice Lattice::operator==(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i == b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f == b.f);
    }
    return ret;
}
Lattice Lattice::operator>=(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i >= b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f >= b.f);
    }
    return ret;
}
Lattice Lattice::operator>(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i > b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f > b.f);
    }
    return ret;
}
Lattice Lattice::operator<=(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i <= b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f <= b.f);
    }
    return ret;
}
Lattice Lattice::operator<(const Lattice &b)
{
    Lattice ret(this->lat, 1);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i < b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.i = (this->f < b.f);
    }
    return ret;
}
Lattice Lattice::operator+(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i + b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.f = (this->f + b.f);
    }
    return ret;
}
Lattice Lattice::operator-(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i - b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.f = (this->f - b.f);
    }
    return ret;
}
Lattice Lattice::operator*(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i * b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.f = (this->f * b.f);
    }
    return ret;
}
Lattice Lattice::operator/(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i / b.i);
    }
    else
    {
        assert(!b.is_i);
        ret.f = (this->f / b.f);
    }
    return ret;
}
Lattice Lattice::operator%(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i * b.i);
    }
    else
    {
        assert(0);
    }
    return ret;
}
Lattice Lattice::operator&(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i & b.i);
    }
    else
    {
        assert(0);
    }
    return ret;
}
Lattice Lattice::operator|(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i | b.i);
    }
    else
    {
        assert(0);
    }
    return ret;
}
Lattice Lattice::operator^(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i ^ b.i);
    }
    else
    {
        assert(0);
    }
    return ret;
}