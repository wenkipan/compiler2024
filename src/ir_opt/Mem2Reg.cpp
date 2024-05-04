#include <ir_opt/Mem2Reg.hpp>

Mem2Reg::~Mem2Reg()
{
    delete DT;
}

int Mem2Reg::get_Instr_id(BasicBlock *BB, Instrution *instr)
{
    int id = 0;
    for (Instrution *it : *(BB->get_instrs()))
    {
        if (it == instr)
            return id;
        id++;
    }
    return -1;
}

bool Mem2Reg::rewriteSingleStoreAlloca(Alloca *alloc)
{
    bool StoringGlobalVal = is_a<Instrution>(OnlyStore->get_value_list()->at(1)->get_val());
    BasicBlock *StoreBB = OnlyStore->get_BB();
    int StoreId = -1;
    UseBlocks[alloc].clear();
    for (Edge *edge : *(alloc->get_user_list()))
    {
        User *U = (User *)edge->get_user();
        Instrution *UserInstr = (Instrution *)U;
        if (UserInstr == OnlyStore)
            continue;
        Load *LI = (Load *)UserInstr;
        if (!StoringGlobalVal)
        {
            if (LI->get_BB() == StoreBB)
            {
                if (StoreId == -1)
                    StoreId = get_Instr_id(StoreBB, OnlyStore);
                if (unsigned(StoreId) > get_Instr_id(StoreBB, LI))
                {
                    UseBlocks[alloc].insert(StoreBB);
                    continue;
                }
            }
            else if (!DT->is_dom(StoreBB, LI->get_BB()))
            {
                UseBlocks[alloc].insert(LI->get_BB());
                continue;
            }
        }
        Value *RepVal = OnlyStore->get_value_list()->at(1)->get_val();
        LI->replaceAllUses(RepVal);
        LI->drop();
    }

    if (!UseBlocks[alloc].empty())
        return false;
    OnlyStore->drop();
    alloc->drop();
    return true;
}

void Mem2Reg::removeFromAllocaList(unsigned &AllocaId)
{
    Allocas[AllocaId] = Allocas.back();
    Allocas.pop_back();
    --AllocaId;
}

bool Mem2Reg::isPromote(Alloca *alloc)
{
    if (dynamic_cast<Ptr *>(alloc->get_type())->get_btype()->get_type() != TypeEnum::F32 and dynamic_cast<Ptr *>(alloc->get_type())->get_btype()->get_type() != TypeEnum::I32)
        return false;
    for (Edge *edge : *(alloc->get_user_list()))
    {
        User *user = (User *)(edge->get_user());
        if (dynamic_cast<Load *>(user) == nullptr && dynamic_cast<Store *>(user) == nullptr)
            return false;
    }
    return true;
}

void Mem2Reg::collectPromotedAllocas(Function *Func)
{
    BasicBlock *BB;
    Allocas.clear();
    for (BasicBlock *BBit : *(Func->get_blocks()))
    {
        BB = BBit;
        for (Instrution *Instrit : *(BB->get_instrs()))
        {
            if (Alloca *alloc = dynamic_cast<Alloca *>(Instrit))
            {
                if (isPromote(alloc))
                    Allocas.push_back(alloc);
            }
        }
    }
}

void Mem2Reg::clear()
{
    OnlyStore = nullptr;
}

void Mem2Reg::analysisAlloca(Alloca *alloc)
{
    for (Edge *edge : *(alloc->get_user_list()))
    {
        User *u = (User *)edge->get_user();
        Instrution *i = dynamic_cast<Instrution *>(u);
        if (Store *SI = dynamic_cast<Store *>(i))
        {
            DefBlocks[alloc].insert(SI->get_BB());
            OnlyStore = SI;
        }
        else
        {
            Load *LI = dynamic_cast<Load *>(i);
            UseBlocks[alloc].insert(LI->get_BB());
        }
    }
}

void Mem2Reg::work(Function *Func)
{
    for (unsigned AllocNum = 0; AllocNum != Allocas.size(); AllocNum++)
    {
        Alloca *AI = Allocas[AllocNum];
        if (AI->get_user_list()->empty())
        {
            AI->drop();
            removeFromAllocaList(AllocNum);
        }
        analysisAlloca(AI);
    }

    std::set<BasicBlock *> PhiSet;
    std::vector<BasicBlock *> W;
    PHINode *phi;
    BasicBlock *X;
    for (Alloca *alloc : Allocas)
    {
        PhiSet.clear();
        W.clear();
        phi = nullptr;
        for (BasicBlock *BB : DefBlocks[alloc])
            W.push_back(BB);
        while (!W.empty())
        {
            X = W.back();
            W.pop_back();
            for (BasicBlock *Y : DT->DomFsBlock[X])
            {
                if (PhiSet.find(Y) == PhiSet.end())
                {
                    phi = new PHINode(Y, alloc->get_type()->get_basic_type(), true);
                    Y->Insert_Phi(phi);
                    PhiSet.insert(Y);
                    PhiMap[Y].insert({phi, alloc});
                    if (std::find(W.begin(), W.end(), Y) == W.end())
                        W.push_back(Y);
                }
            }
        }
    }

    std::vector<Instrution *> InstrRemoveList;
    std::vector<std::pair<BasicBlock *, std::map<Alloca *, Value *>>> Worklist;
    std::set<BasicBlock *> VisSet;
    BasicBlock *SuccBB, *BB;
    std::map<Alloca *, Value *> IncommingValues;
    Instrution *Instr;
    Worklist.push_back({Func->get_entryBB(), {}});
    for (Alloca *alloc : Allocas)
    {
        Value *_pval = new Value(alloc->get_type()->get_basic_type());
        Func->value_pushBack(_pval);
        Worklist[0].second[alloc] = new Assign(InstrutionEnum::Assign, _pval, Func->get_entryBB(), true);
    }
    while (!Worklist.empty())
    {
        BB = Worklist.back().first;
        IncommingValues = Worklist.back().second;
        Worklist.pop_back();
        if (VisSet.find(BB) != VisSet.end())
            continue;
        else
            VisSet.insert(BB);
        int pos = 0;
        for (PHINode *Phi : *(BB->get_phis()))
        {
            if (PhiMap[BB].find(Phi) == PhiMap[BB].end())
                continue;
            IncommingValues[PhiMap[BB][Phi]] = Phi;
        }
        for (Instrution *it : *(BB->get_instrs()))
        {
            pos++;
            Instr = it;
            if (Alloca *AI = dynamic_cast<Alloca *>(it))
            {
                if (std::find(Allocas.begin(), Allocas.end(), AI) == Allocas.end())
                    continue;
                InstrRemoveList.push_back(Instr);
            }
            else if (Load *LI = dynamic_cast<Load *>(it))
            {
                Alloca *AI = dynamic_cast<Alloca *>(LI->get_addr());
                if (!AI)
                    continue;
                if (std::find(Allocas.begin(), Allocas.end(), AI) != Allocas.end())
                {
                    if (IncommingValues.find(AI) == IncommingValues.end())
                    {
                        Value *_pval = new Value(AI->get_type()->get_basic_type());
                        Func->value_pushBack(_pval);
                        IncommingValues[AI] = new Assign(InstrutionEnum::Assign, _pval, BB, true);
                    }
                    LI->replaceAllUses(IncommingValues[AI]);
                    InstrRemoveList.push_back(Instr);
                }
            }
            else if (Store *SI = dynamic_cast<Store *>(it))
            {
                Alloca *AI = dynamic_cast<Alloca *>(SI->get_addr());
                if (!AI)
                    continue;
                if (std::find(Allocas.begin(), Allocas.end(), AI) == Allocas.end())
                    continue;
                if (dynamic_cast<Instrution *>(SI->get_src()))
                {
                    IncommingValues[AI] = SI->get_src();
                    InstrRemoveList.push_back(Instr);
                }
                else
                {
                    IncommingValues[AI] = new Assign(InstrutionEnum::Assign, SI->get_src(), BB, true);
                    BB->Ins_set(pos - 1, (Instrution *)IncommingValues[AI]);
                    Instr->drop();
                }
            }
        }
        for (Edge *edge : *(BB->get_user_list()))
        {
            SuccBB = (BasicBlock *)edge->get_user();
            Worklist.push_back({SuccBB, IncommingValues});
            for (PHINode *Phi : *(SuccBB->get_phis()))
            {
                if (PhiMap[SuccBB].find(Phi) == PhiMap[SuccBB].end())
                    continue;
                if (IncommingValues[PhiMap[SuccBB][Phi]])
                    Phi->addIncoming(IncommingValues[PhiMap[SuccBB][Phi]], BB);
            }
        }
    }
    while (!InstrRemoveList.empty())
    {
        Instr = InstrRemoveList.back();
        InstrRemoveList.pop_back();
        Instr->drop();
    }

    for (auto &it1 : PhiMap)
    {
        for (auto &it : it1.second)
        {
            if (it.first->get_user_list()->empty())
                it.first->drop();
        }
    }
}

bool Mem2Reg::run(Function *Func)
{
    while (true)
    {
        collectPromotedAllocas(Func);
        if (Allocas.empty())
            break;
        DefBlocks.clear();
        UseBlocks.clear();
        PhiMap.clear();
        DT = new DomTree(Func);
        DT->MakeDom();
        DT->get_DF();
        work(Func);
    }
    return true;
}