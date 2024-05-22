#include <ir_opt/LivenessAnalysis.hpp>

void LivenessAnalysis::init()
{
    int allocaCounter = 0;
    for (auto arg : *(parent->get_params()))
    {
        Value *val = dynamic_cast<Value *>(arg);
        ValueIdMap[val] = allocaCounter++;
        Vals.push_back(val);
    }
    for (auto bb : BBs)
    {
        for (auto ins : *(bb->get_instrutions()))
        {
            Value *val = dynamic_cast<Value *>(ins);
            if (val->get_type()->get_type() != TypeEnum::Void)
            {
                ValueIdMap[val] = allocaCounter++;
                Vals.push_back(val);
            }
        }
    }
    for (auto bb : BBs)
    {
        InSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        OutSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        DefSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
        UseSet.insert(std::make_pair(bb, bit_vector(allocaCounter)));
    }
}

bool LivenessAnalysis::work_BB(BasicBlock *bb)
{
    bool change = false;
    for (Edge *edge : *(bb->get_user_list()))
    {
        BasicBlock *v = dynamic_cast<BasicBlock *>(edge->get_user());
        OutSet[bb] = (OutSet[bb] | InSet[v]);
    }
    bit_vector tmp = InSet[bb];
    InSet[bb] = UseSet[bb] | (OutSet[bb] & ~DefSet[bb]);
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
            for (auto edge : *(phi->get_value_list()))
            {
                Value *tmp = edge->get_val();
                if (ValueIdMap.find(tmp) != ValueIdMap.end() && !DefSet[bb].at(ValueIdMap[tmp]))
                    UseSet[bb].set(ValueIdMap[tmp], true);
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
                if (ValueIdMap.find(tmp) != ValueIdMap.end() && !DefSet[bb].at(ValueIdMap[tmp]))
                    UseSet[bb].set(ValueIdMap[tmp], true);
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
    for (auto bb : BBs)
    {
        bb->print();
        for (auto val : Vals)
        {
            printf("%d %d : ", InSet[bb].at(ValueIdMap[val]), OutSet[bb].at(ValueIdMap[val]));
            val->print();
        }
    }
}