#include <ir_opt/LivenessAnalysis.hpp>

void LivenessAnalysis::init()
{
    int allocaCounter = 0;
    Vals.clear();
    ValueIdMap.clear();
    is_float.clear();
    for (auto arg : *(parent->get_params()))
    {
        Value *val = dynamic_cast<Value *>(arg);
        ValueIdMap[val] = allocaCounter++;
        Vals.push_back(val);
        is_float.push_back(val->get_type()->get_type() == TypeEnum::F32);
    }
    for (auto bb : BBs)
    {
        for (auto phi : *(bb->get_phinodes()))
        {
            ValueIdMap[phi] = allocaCounter++;
            Vals.push_back(phi);
            is_float.push_back(phi->get_type()->get_type() == TypeEnum::F32);
        }
        for (auto ins : *(bb->get_instrutions()))
        {
            Value *val = dynamic_cast<Value *>(ins);
            if (val->get_type()->get_type() != TypeEnum::Void && !dynamic_cast<Alloca *>(ins) && !dynamic_cast<Ret *>(ins))
            {
                ValueIdMap[val] = allocaCounter++;
                Vals.push_back(val);
                is_float.push_back(val->get_type()->get_type() == TypeEnum::F32);
            }
        }
    }
    InSet.clear();
    OutSet.clear();
    DefSet.clear();
    UseSet.clear();
    InDis.clear();
    OutDis.clear();
    for (auto bb : BBs)
    {
        InSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        OutSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        DefSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        UseSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        InDis.insert(std::make_pair(bb, std::vector<int>(allocaCounter, INF)));
        OutDis.insert(std::make_pair(bb, std::vector<int>(allocaCounter, INF)));
    }
}

bool LivenessAnalysis::work_BB(BasicBlock *bb)
{
    bool change = false;
    for (Edge *edge : *(bb->get_user_list()))
    {
        BasicBlock *v = dynamic_cast<BasicBlock *>(edge->get_user());
        OutSet[bb] = (OutSet[bb] | InSet[v]);
        for (int i = 0; i < Vals.size(); i++)
            OutDis[bb][i] = std::min(OutDis[bb][i], InDis[v][i]);
    }
    bit_vector tmp = InSet[bb];
    InSet[bb] = UseSet[bb] | (OutSet[bb] & ~DefSet[bb]);
    for (int i = 0; i < Vals.size(); i++)
        InDis[bb][i] = OutDis[bb][i] + bb->get_instrs()->size();
    for (int i = 0; i < bb->get_instrs()->size(); i++)
    {
        for (Edge *edge : *(bb->get_instrs()->at(i)->get_value_list()))
        {
            Value *val = edge->get_val();
            if (ValueIdMap.find(val) != ValueIdMap.end())
            {
                InDis[bb][ValueIdMap[val]] = std::min(InDis[bb][ValueIdMap[val]], i);
            }
        }
    }
    if (!change && InSet[bb] != tmp)
        change = true;
    return change;
}

void LivenessAnalysis::DefAndUseAnalysis()
{
    for (auto bb : BBs)
    {
        for (auto phi : *(bb->get_phinodes()))
        {
            Value *val = dynamic_cast<Value *>(phi);
            for (auto it : *(phi->get_valueMap()))
            {
                Value *tmp = it.second->get_val();
                if (ValueIdMap.find(tmp) != ValueIdMap.end() && !DefSet[bb].at(ValueIdMap[tmp]))
                    OutSet[it.first].set(ValueIdMap[tmp], true);
            }
            if (ValueIdMap.find(val) != ValueIdMap.end())
                DefSet[bb].set(ValueIdMap[val], true);
        }
        for (auto ins : *(bb->get_instrutions()))
        {
            Value *val = dynamic_cast<Value *>(ins);
            for (auto edge : *(ins->get_value_list()))
            {
                Value *tmp = edge->get_val();
                if (ValueIdMap.find(tmp) != ValueIdMap.end())
                {
                    if (!DefSet[bb].at(ValueIdMap[tmp]))
                        UseSet[bb].set(ValueIdMap[tmp], true);
                }
            }
            if (ValueIdMap.find(val) != ValueIdMap.end())
                DefSet[bb].set(ValueIdMap[val], true);
        }
    }
}

void LivenessAnalysis::dfs(BasicBlock *u)
{
    for (auto edge : *(u->get_user_list()))
    {
        BasicBlock *v = dynamic_cast<BasicBlock *>(edge->get_user());
        if (vis.find(v) == vis.end())
        {
            vis.insert(v);
            dfs(v);
        }
    }
    BBs.push_back(u);
}
void LivenessAnalysis::run(Function *func)
{
    parent = func;
    BBs.clear();
    vis.clear();
    dfs(func->get_entryBB());
    init();
    DefAndUseAnalysis();
    bool change = true;
    while (change)
    {
        change = false;
        for (auto bb : BBs)
        {
            change |= work_BB(bb);
        }
    }
    /*for (auto bb : BBs)
    {
        bb->print();
        for (auto val : Vals)
        {
            printf("%d %d : ", InSet[bb].at(ValueIdMap[val]), OutSet[bb].at(ValueIdMap[val]));
            val->print();
        }
    }*/
}