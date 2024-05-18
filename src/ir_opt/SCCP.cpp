#include "ir/BasicBlock.hpp"
#include "ir/Constant.hpp"
#include "ir/Function.hpp"
#include "ir/Instrution.hpp"
#include <ir_opt/SCCP.hpp>
#include <fstream>
#include <ostream>
#include <set>
void SCCP::run(Function *func)
{
    function = func;
    init();
    while (!cfg_worklist.empty() || !ssa_worklist.empty())
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
            // cur_bb->print();
            for (auto phi : *cur_bb->get_phinodes())
                visit_phi(phi);

            // visit exp when block havent been visit
            assert(executed_block_map.find(cur_bb) != executed_block_map.end());
            if (executed_block_map[cur_bb] == true)
                continue;
            executed_block_map[cur_bb] = true;

            // printf("first enter this block\n");
            for (auto instr : *cur_bb->get_instrutions())
                visit_exp(instr);

            // only have one out-edge(Jmp)
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
                visit_phi((PHINode *)instr);
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
    fakeedge->drop();
    delete fakeBlock;
    // print();
    do_sccp();
}

void SCCP::init()
{
    // entry bb cfg_worklist
    assert((function->get_entryBB()));
    fakeBlock = new BasicBlock;
    fakeedge = new Edge(function->get_entryBB(), fakeBlock); // TODO:delete this edge
    cfg_worklist.push(fakeedge);
    executed_edge_map.emplace(fakeedge, false);
    //  executed_block_edge_map init
    for (BasicBlock *bb : *function->get_blocks())
    {
        // bb->print();
        if (bb->get_instrutions()->size() == 0)
            continue;
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
    // printf("do_sccp begin\n");
    do_sccp_drop_unexecuted_blocks();
    for (auto BB : *function->get_blocks())
    {
        assert(executed_block_map.find(BB) != executed_block_map.end());
        if (executed_block_map.find(BB)->second == false)
        {
            // BB->print();
            for (auto edge : *BB->get_value_list())
                assert(executed_edge_map.find(edge)->second == false);
            continue;
        }

        for (auto instr : *BB->get_instrs())
        {
            if (latticemap.find(instr) == latticemap.end())
                continue;
            Lattice l = get_lattice_from_map(instr);
            std::vector<Edge *> edges;
            switch (l.lat)
            {
            case Lattice::Lat::UNDEF:
                instr->print();
                assert(0);
                break;
            case Lattice::Lat::CONST:
                edges = *instr->get_user_list();
                if (l.is_i)
                {
                    ConstantI32 *consti = new ConstantI32(l.i);
                    function->value_pushBack(consti);
                    for (auto edge : edges)
                    {
                        edge->set_val(consti);
                        // edge->get_user()->print();
                    }
                }
                else
                {
                    ConstantF32 *constf = new ConstantF32(l.f);
                    function->value_pushBack(constf);
                    for (auto edge : edges)
                    {
                        edge->set_val(constf);
                    }
                }
                instr->get_user_list()->clear();
                break;
            case Lattice::Lat::NAC:
                if (is_a<Assign>(instr))
                {
                    edges = *instr->get_user_list();
                    for (auto edge : edges)
                        edge->set_val(instr->get_value_list()->at(0)->get_val());
                    instr->get_user_list()->clear();
                }
                break;
            default:
                assert(0);
                break;
            }
        }
        for (auto phi : *BB->get_phinodes())
        {
            std::vector<Edge *> edges;
            Lattice l = get_lattice_from_map(phi);
            switch (l.lat)
            {
            case Lattice::Lat::UNDEF:
                assert(0);
                break;
            case Lattice::Lat::CONST:
                edges = *phi->get_user_list();
                if (l.is_i)
                {
                    ConstantI32 *consti = new ConstantI32(l.i);
                    function->value_pushBack(consti);
                    for (auto edge : edges)
                    {
                        edge->set_val(consti);
                    }
                }
                else
                {
                    ConstantF32 *constf = new ConstantF32(l.f);
                    function->value_pushBack(constf);
                    for (auto edge : edges)
                    {
                        edge->set_val(constf);
                    }
                }
                phi->get_user_list()->clear();
                break;
            case Lattice::Lat::NAC:
                break;
            default:
                assert(0);
                break;
            }
        }
    }
}
void SCCP::do_sccp_drop_unexecuted_blocks()
{
    std::queue<Edge *> droplist;
    for (auto BB : *function->get_blocks())
    {
        assert(executed_block_map.find(BB) != executed_block_map.end());
        if (executed_block_map[BB] != true)
            continue;
        Instrution *last = BB->get_last_instrution();
        if (last->isBranch())
        {
            auto edges = *BB->get_user_list();
            assert(edges.size() == 2);
            Value *cond = ((Branch *)last)->get_cond();
            Lattice condlattice = latticemap.find(cond)->second;
            BasicBlock *truebb = (BasicBlock *)edges[0]->get_user();
            BasicBlock *falsebb = (BasicBlock *)edges[1]->get_user();
            switch (condlattice.lat)
            {
            case Lattice::Lat::UNDEF:
                assert(0);
                break;
            case Lattice::Lat::CONST:
                assert(condlattice.is_i);
                last->drop();
                assert(is_a<Instrution>(cond));
                ((Instrution *)cond)->drop();
                droplist.push(edges[0]);
                droplist.push(edges[1]);
                BB->get_user_list()->clear();
                if (condlattice.i)
                {
                    assert(executed_edge_map.find(edges[1])->second == false);
                    for (auto phi : *falsebb->get_phinodes())
                        phi->eraseIncoming(BB);
                    new Jmp((BasicBlock *)truebb, BB);
                }
                else
                {
                    assert(executed_edge_map.find(edges[0])->second == false);
                    for (auto phi : *truebb->get_phinodes())
                        phi->eraseIncoming(BB);
                    new Jmp((BasicBlock *)falsebb, BB);
                }
                break;
            case Lattice::Lat::NAC:
                assert(executed_edge_map.find(edges[0])->second == true);
                assert(executed_edge_map.find(edges[1])->second == true);
                assert(executed_block_map.find(truebb)->second == true);
                assert(executed_block_map.find(falsebb)->second == true);
                break;
            default:
                assert(0);
                break;
            }
        }
        else if (last->isJmp())
            assert(executed_block_map.find(successors(BB, 0))->second == true);
        else
            assert(last->isReturn());
    }
    drop_all_edges(droplist);
}

Lattice SCCP::get_lattice_from_map(Value *v)
{
    auto it = latticemap.find(v);
    assert(it != latticemap.end());
    return it->second;
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
void SCCP::visit_phi(PHINode *phi)
{
    Lattice tmp = get_lattice_from_map(phi);
    if (tmp.lat == Lattice::Lat::NAC)
        return;
    for (auto kv : *phi->get_valueMap())
    {
        BasicBlock *bb = kv.first;
        assert(executed_block_map.find(bb) != executed_block_map.end());
        if (executed_block_map[bb] == false)
            continue;

        Lattice lat_for_value = get_lattice_from_map(kv.second->get_val());
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
        // about Branch ,it always dealing cond , branch itself is useless
        auto cond = ((Branch *)instrution)->get_cond();
        Lattice cond_lattice = get_lattice_from_map(cond);
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
        latticemap.find(instrution)->second = cond_lattice;
    }
    else if (instrution->isBinary())
        need_add_ssa = visit_binary(instrution);
    else if (instrution->isUnary())
        need_add_ssa = visit_unary(instrution);
    else if (instrution->isGEP())
        need_add_ssa = visit_GEP(instrution);
    else if (instrution->isCmp())
        need_add_ssa = visit_cmp(instrution);
    else if (instrution->isLoad() || instrution->isCall())
    { // must be NAC
        Lattice::Lat tmp = latticemap.find(instrution)->second.lat;
        if (tmp != Lattice::Lat::NAC)
        {
            need_add_ssa = 1;
            latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
        }
    }
    else if (instrution->isReturn() || instrution->isStore() || instrution->isAlloca() || instrution->isJmp())
    { // dont have user ,set to NAC
        latticemap.find(instrution)->second.lat = Lattice::Lat::NAC;
    }
    else
        assert(0);

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
    auto lat1 = get_lattice_from_map((*value_list)[0]->get_val());
    auto lat2 = get_lattice_from_map((*value_list)[1]->get_val());
    return latticemap.find(instr)->second.exp_insersect(lat1, lat2, instr->get_Instrtype());
}
bool SCCP::visit_cmp(Instrution *instr)
{
    auto value_list = instr->get_value_list();
    assert(value_list->size() == 2);
    auto lat1 = get_lattice_from_map((*value_list)[0]->get_val());
    auto lat2 = get_lattice_from_map((*value_list)[1]->get_val());
    return latticemap.find(instr)->second.exp_insersect(lat1, lat2, instr->get_Instrtype());
}
bool SCCP::visit_unary(Instrution *instr)
{
    assert(is_a<Unary>(instr));
    Value *src = (*instr->get_value_list())[0]->get_val();
    Lattice tmp = get_lattice_from_map(src);
    Lattice instr_lattice = latticemap.find(instr)->second;
    if (tmp.lat == Lattice::Lat::UNDEF)
    {
        assert(0);
    }
    else if (tmp.lat == Lattice::Lat::NAC)
    {
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            latticemap.find(instr)->second.lat = Lattice::Lat::NAC;
            return true;
        }
        return false;
    }

    assert(tmp.lat == Lattice::Lat::CONST);
    switch (instr->get_Instrtype())
    {
    case InstrutionEnum::MINUS: // problem!!!
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
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            assert(tmp.is_i);
            tmp.is_i = 0;
            tmp.f = (int)tmp.i;
            latticemap.find(instr)->second = tmp;
            return true;
        }
        return false;
    case InstrutionEnum::Assign:
        if (is_lat_lower(instr_lattice.lat, tmp.lat))
        {
            latticemap.find(instr)->second = tmp;
            return true;
        }
        return false;
    // case InstrutionEnum::AddSP: // no user so set to NAC?
    //     latticemap.find(instr)->second.lat = Lattice::Lat::NAC;
    //     return false;
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
            tmp = lat1;
        else if (!lat1.is_i && (lat1.f == lat2.f))
            tmp = lat1;
        else
            tmp.lat = Lat::NAC;
    }
    else if (lat1.lat == Lat::CONST)
        tmp = lat1;
    else if (lat2.lat == Lat::CONST)
        tmp = lat2;
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

void SCCP::print()
{
    FILE *fp;
    fp = freopen("doc/draw_source/SCCPBLOCK.txt", "w+", stdout);
    for (auto bb : *function->get_blocks())
    {
        // printf("BB::%d\n", bb->get_ID());
        for (auto userbb : *bb->get_user_list())
        {
            assert(is_a<BasicBlock>(userbb->get_user()));
            std::cout << bb->get_ID() << ',' << (executed_block_map.find(bb)->second ? "true" : "false") << "->"
                      << userbb->get_user()->get_ID() << ',' << (executed_block_map.find((BasicBlock *)userbb->get_user())->second ? "true" : "false") << std::endl;
        }
    }
    fclose(fp);
    fp = freopen("doc/draw_source/SCCPINSTR.txt", "w+", stdout);
    for (auto bb : *function->get_blocks())
    {
        for (auto instr : *bb->get_instrs())
        {
            // printf("instr::%d\n", instr->get_ID());
            for (auto user : *instr->get_user_list())
            {
                assert(latticemap.find(instr) != latticemap.end());
                assert(latticemap.find(user->get_user()) != latticemap.end());
                Lattice lattice = latticemap.find(instr)->second;
                Lattice l2 = latticemap.find(user->get_user())->second;
                std::cout << instr->get_ID() << ',' << lattice << "->"
                          << user->get_user()->get_ID() << ',' << l2 << std::endl;
            }
        }
    }
    fclose(fp);
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
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i != b.i);
    else
        ret.i = (this->f != b.f);
    return ret;
}
Lattice Lattice::operator==(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i == b.i);
    else
        ret.i = (this->f == b.f);
    return ret;
}
Lattice Lattice::operator>=(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i >= b.i);
    else
        ret.i = (this->f >= b.f);
    return ret;
}
Lattice Lattice::operator>(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i > b.i);
    else
        ret.i = (this->f > b.f);
    return ret;
}
Lattice Lattice::operator<=(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i <= b.i);
    else
        ret.i = (this->f <= b.f);
    return ret;
}
Lattice Lattice::operator<(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, 1);
    if (is_i)
        ret.i = (this->i < b.i);
    else
        ret.i = (this->f < b.f);
    return ret;
}
Lattice Lattice::operator+(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, this->is_i);
    if (is_i)
        ret.i = (this->i + b.i);
    else
        ret.f = (this->f + b.f);
    return ret;
}
Lattice Lattice::operator-(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, this->is_i);
    if (is_i)
        ret.i = (this->i - b.i);
    else
        ret.f = (this->f - b.f);
    return ret;
}
Lattice Lattice::operator*(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, this->is_i);
    if (is_i)
        ret.i = (this->i * b.i);
    else
        ret.f = (this->f * b.f);
    return ret;
}
Lattice Lattice::operator/(const Lattice &b)
{
    assert(is_i == b.is_i);
    Lattice ret(this->lat, this->is_i);
    if (is_i)
        ret.i = (this->i / b.i);
    else
        ret.f = (this->f / b.f);
    return ret;
}
Lattice Lattice::operator%(const Lattice &b)
{
    Lattice ret(this->lat, this->is_i);
    if (is_i)
    {
        assert(b.is_i);
        ret.i = (this->i % b.i);
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