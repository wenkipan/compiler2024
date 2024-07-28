#include "../../include/ir_opt/loopFullunroll.hpp"
#include "../../include/ir_opt/IRCopy.hpp"
#include "../../include/ir_opt/LCSSA.hpp"

#define MustDo(it, list_) ((it) == (list_)->end() ? Assert_() : true)

static inline bool Assert_()
{
    assert(0);
    return false;
}

static inline void ResetVal_(PHINode *phi, Value *val, BasicBlock *BB, IRCopy &copyer)
{
    PHINode *cphi = (PHINode *)copyer.get_mapval(phi);
    BasicBlock *cBB = copyer.get_mapbb(BB);
    cphi->eraseIncoming(cBB);
    cphi->addIncoming(val, cBB);
}

void loopFullunroll::SetVal(PHINode *phi, IRCopy &copyer, Loop *loop)
{
    auto map_ = phi->get_valueMap();
    for (auto it_ : *map_)
    {
        assert(loop->is_BBinLoop(it_.first) || loop->get_prev() == it_.first);
        Value *val = it_.second->get_val();
        if (is_a<Param>(val) || is_a<Constant>(val))
            ResetVal_(phi, val, it_.first, copyer);
        else if (is_a<Instrution>(val))
        {
            Instrution *instr_ = (Instrution *)val;
            BasicBlock *BB_ = instr_->get_parent();
            if (!loop->is_BBinLoop(BB_))
                ResetVal_(phi, val, it_.first, copyer);
        }
        else
            assert(0);
    }
}

void loopFullunroll::SetUser(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list_ = instr->get_user_list();
    for (auto it_ : *list_)
    {
        Instrution *T = (Instrution *)it_->get_user();
        if (loop->is_BBinLoop(T->get_parent()))
            continue;
        assert(is_a<PHINode>(it_->get_user()));
        PHINode *user = (PHINode *)it_->get_user();
        BasicBlock *BB = user->get_edge_income_block(it_);
        assert(loop->is_BBinLoop(BB));
        PHINode *cuser = (PHINode *)copyer.get_mapval(user);
        BasicBlock *cBB = copyer.get_mapbb(BB);
        cuser->eraseIncoming(cBB);
        user->addIncoming(copyer.get_mapval(instr), cBB);
    }
}

static inline void ResetVal_(Instrution *instr, Value *val, IRCopy &copyer)
{
    Instrution *cinstr = (Instrution *)copyer.get_mapval(instr);
    Value *cval = copyer.get_mapval(val);
    auto list_ = cval->get_user_list();
    for (auto it = list_->begin(); MustDo(it, list_); ++it)
        if ((*it)->get_user() == cinstr)
        {
            (*it)->set_val(val);
            list_->erase(it);
            break;
        }
}

void loopFullunroll::SetVal(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list_ = instr->get_value_list();
    for (auto it : *list_)
    {
        Value *val = it->get_val();
        if (is_a<Param>(val) || is_a<Constant>(val))
            ResetVal_(instr, val, copyer);
        else if (is_a<Instrution>(val))
        {
            Instrution *instr_ = (Instrution *)val;
            BasicBlock *BB_ = instr_->get_parent();
            if (!loop->is_BBinLoop(BB_))
                ResetVal_(instr, val, copyer);
        }
        else if (is_a<GlobalValue>(val))
            continue;
        else
            assert(0);
    }
}

static inline void _dealCond(BasicBlock *_exiting, BasicBlock *nextBB, Loop *loop, BasicBlock *nwexiting)
{
    assert(nwexiting->get_last_instrution()->isBranch());
    Branch *p_branch = ((Branch *)nwexiting->get_last_instrution());
    nwexiting->get_instrs()->pop_back();
    Instrution *p_cond = (Instrution *)p_branch->get_cond();
    assert(p_cond == nwexiting->get_last_instrution());
    p_cond->get_user_list()->clear();
    p_cond->drop();
    Constant *const1 = new ConstantI32(1), *const2 = new ConstantI32(0);
    _exiting->get_func()->value_pushBack(const1), _exiting->get_func()->value_pushBack(const2);
    Edge *newEdge = new Edge(nextBB, nwexiting);
    assert(nwexiting->get_user_list()->back() == newEdge);
    nwexiting->get_user_list()->pop_back();
    Edge *delEdge = nullptr;
    Branch *tmp = nullptr;
    if (nwexiting == _exiting)
        tmp = p_branch;
    else
        tmp = (Branch *)_exiting->get_last_instrution();
    if (loop->is_BBinLoop(tmp->get_trueBB()))
    {
        p_cond = new Cmp(InstrutionEnum::IEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[1];
        (*nwexiting->get_user_list())[1] = newEdge;
    }
    else
    {
        p_cond = new Cmp(InstrutionEnum::INEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[0];
        (*nwexiting->get_user_list())[0] = newEdge;
    }
    auto DelEdges = delEdge->get_user()->get_value_list();
    for (auto it = DelEdges->begin(); it != DelEdges->end(); ++it)
        if ((*it) == delEdge)
        {
            DelEdges->erase(it);
            break;
        }
    delete delEdge;
    assert(p_branch->get_value_list()->size() == 1);
    (*p_branch->get_value_list()->begin())->set_val(p_cond);
    nwexiting->get_instrs()->push_back(p_branch);

    BasicBlock *exit = *loop->get_exits()->begin();
    assert(loop->get_exits()->size() == 1);
    for (PHINode *Phi : *exit->get_phinodes())
        Phi->eraseIncoming(nwexiting);
}

static inline void _dealExit(BasicBlock *lpexiting, Loop *loop, IRCopy &copyer)
{
    BasicBlock *nwexiting = copyer.get_mapbb(lpexiting);
    Branch *p_branch = ((Branch *)nwexiting->get_last_instrution());
    nwexiting->get_instrs()->pop_back();
    Instrution *p_cond = (Instrution *)p_branch->get_cond();
    assert(p_cond == nwexiting->get_last_instrution());
    p_cond->get_user_list()->clear();
    p_cond->drop();
    Constant *const1 = new ConstantI32(1), *const2 = new ConstantI32(0);
    lpexiting->get_func()->value_pushBack(const1), lpexiting->get_func()->value_pushBack(const2);
    BasicBlock *exitBB = *loop->get_exits()->begin();
    Edge *newEdge = new Edge(exitBB, nwexiting);
    nwexiting->get_user_list()->pop_back();
    Edge *delEdge = nullptr;
    if (loop->is_BBinLoop(((Branch *)lpexiting->get_last_instrution())->get_trueBB()))
    {
        p_cond = new Cmp(InstrutionEnum::IEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[1];
        (*nwexiting->get_user_list())[1] = newEdge;
    }
    else
    {
        p_cond = new Cmp(InstrutionEnum::INEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[0];
        (*nwexiting->get_user_list())[0] = newEdge;
    }
    auto DelEdges = delEdge->get_user()->get_value_list();
    for (auto it = DelEdges->begin(); it != DelEdges->end(); ++it)
        if ((*it) == delEdge)
        {
            DelEdges->erase(it);
            break;
        }
    delete delEdge;
    (*p_branch->get_value_list()->begin())->set_val(p_cond);
    nwexiting->get_instrs()->push_back(p_branch);
    assert(p_branch->get_value_list()->size() == 1);
}

void loopFullunroll::FullUnroll(Loop *loop, int times, Function *p_func)
{
    times = times - 2;
    Function *func[128];
    IRCopy copyer[128];
    for (int i = 0; i <= times; ++i)
        func[i] = copyer[i].copy_func(p_func);
    for (int i = 0; i <= times; ++i)
    {
        std::vector<Value *> addvals;
        std::vector<BasicBlock *> addBBs;
        for (BasicBlock *BB : *loop->get_BBs())
        {
            addBBs.emplace_back(copyer[i].get_mapbb(BB));
            for (PHINode *phi : *BB->get_phinodes())
            {
                addvals.emplace_back(copyer[i].get_mapval(phi));
                SetVal(phi, copyer[i], loop);
                SetUser(phi, copyer[i], loop);
            }
            for (Instrution *instr : *BB->get_instrutions())
            {
                addvals.emplace_back(copyer[i].get_mapval(instr));
                SetVal(instr, copyer[i], loop);
                SetUser(instr, copyer[i], loop);
            }
        }
        for (BasicBlock *BB : *loop->get_BBs())
        {
            auto list = BB->get_user_list();
            for (auto edge : *list)
            {
                BasicBlock *nextBB = (BasicBlock *)edge->get_user();
                if (loop->is_BBinLoop(nextBB))
                    continue;
                BasicBlock *cnextBB = copyer[i].get_mapbb(nextBB);
                auto users = copyer[i].get_mapbb(BB)->get_user_list();
                for (auto cedge : *users)
                {
                    if (cedge->get_user() == cnextBB)
                    {
                        cedge->set_user(nextBB);
                    }
                }
            }
        }
        for (auto it : addBBs)
        {
            auto list_ = func[i]->get_blocks();
            for (auto del = list_->begin(); MustDo(del, list_); ++del)
                if ((*del) == it)
                {
                    it->Set_parent(p_func);
                    p_func->get_blocks()->emplace_back(it);
                    list_->erase(del);
                    break;
                }
        }
        for (auto it : addvals)
        {
            auto list_ = func[i]->get_Values();
            for (auto del = list_->begin(); MustDo(del, list_); ++del)
                if ((*del) == it)
                {
                    p_func->get_Values()->emplace_back(it);
                    list_->erase(del);
                    break;
                }
        }
    }
    BasicBlock *lpexiting = *loop->get_exitings()->begin();
    BasicBlock *lplatch = *loop->get_latchs()->begin();
    for (int i = 0; i < times; ++i)
    {
        BasicBlock *nwexiting = copyer[i].get_mapbb(lpexiting);
        BasicBlock *nextHeader = copyer[i + 1].get_mapbb(loop->get_header());
        BasicBlock *nextPrev = copyer[i + 1].get_mapbb(loop->get_prev());
        _dealCond(lpexiting, nextHeader, loop, copyer[i].get_mapbb(lpexiting));
        for (PHINode *Phi : *loop->get_header()->get_phinodes())
        {
            PHINode *nextPhi = ((PHINode *)copyer[i + 1].get_mapval(Phi));
            nextPhi->eraseIncoming(nextPrev);
            nextPhi->addIncoming(copyer[i].get_mapval(Phi->get_valueMap()->find(lplatch)->second->get_val()), nwexiting);
        }
        for (auto it = nextHeader->get_value_list()->begin(); it != nextHeader->get_value_list()->end(); ++it)
            if ((*it)->get_val() == nextPrev)
            {
                nextHeader->get_value_list()->erase(it);
                break;
            }
    }
    _dealExit(lpexiting, loop, copyer[times]);
    _dealCond(lpexiting, copyer[0].get_mapbb(loop->get_header()), loop, lpexiting);
    BasicBlock *nextHeader = copyer[0].get_mapbb(loop->get_header());
    BasicBlock *nextPrev = copyer[0].get_mapbb(loop->get_prev());
    for (PHINode *Phi : *loop->get_header()->get_phinodes())
    {
        PHINode *nextPhi = ((PHINode *)copyer[0].get_mapval(Phi));
        nextPhi->eraseIncoming(nextPrev);
        nextPhi->addIncoming(Phi->get_valueMap()->find(lplatch)->second->get_val(), lpexiting);
    }

    for (auto it = nextHeader->get_value_list()->begin(); it != nextHeader->get_value_list()->end(); ++it)
        if ((*it)->get_val() == nextPrev)
        {
            nextHeader->get_value_list()->erase(it);
            break;
        }
    for (int i = 0; i <= times; ++i)
    {
        auto list = func[i]->get_Values();
        for (Value *val : *list)
        {
            auto edges = val->get_value_list();
            for (auto edge : *edges)
            {
                Value *value = edge->get_val();
                if (is_a<GlobalValue>(value))
                {
                    auto users = value->get_user_list();
                    for (auto it = users->begin(); it != users->end(); ++it)
                    {
                        if ((*it) == edge)
                        {
                            delete edge;
                            users->erase(it);
                            break;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i <= times; ++i)
        delete func[i];
}

static bool _checkLoop(Loop *loop)
{
    bool re = true;
    for (Loop *son : *loop->get_lpsons())
    {
        re = re & _checkLoop(son);
    }
    if (loop->get_exitings()->size() == 1 && loop->get_latchs()->size() == 1 && (*loop->get_exitings()->begin()) == (*loop->get_latchs()->begin()))
        return re;
    else
        return false;
}

static inline int _checkFull(Loop *loop, SCEV *scev)
{

    int re = -1;
    if (!loop->is_stepSet())
        return -1;
    SCEVEXP *exp = scev->find_exp(loop->get_lpStep());
    if (exp == nullptr)
        return -1;
    auto dims = exp->get_dims();
    if (!(*dims)[2].empty())
        return -1;
    int StepBegin = 0x3f3f3f3f, StepNum = 0x3f3f3f3f, StepEnd = 0x3f3f3f3f;
    if ((*dims)[1].size() != 1 || (*dims)[0].size() != 1)
        return -1;
    auto it = (*dims)[1].begin();
    if (is_a<ConstantI32>(it->first))
    {
        assert(it->second != SCEVType::MUL);
        StepNum = *((ConstantI32 *)it->first)->get_i32().begin();
        if (it->second == SCEVType::SUB)
            StepNum *= -1;
    }
    else
        return -1;

    it = (*dims)[0].begin();
    if (is_a<PHINode>(it->first))
    {

        auto valMap = ((PHINode *)it->first)->get_valueMap();
        if (valMap->size() != 1)
            return -1;
        auto It = valMap->begin()->second->get_val();
        ConstantI32 *Cnum = dynamic_cast<ConstantI32 *>(It);
        if (Cnum != nullptr)
        {
            StepBegin = *Cnum->get_i32().begin();
            assert(it->second == SCEVType::ADD);
        }
        else
            return -1;
    }
    else
        return -1;

    if (is_a<ConstantI32>(loop->get_lpEnd()))
        StepEnd = *((ConstantI32 *)loop->get_lpEnd())->get_i32().begin();
    else
        return -1;

    int _type = loop->get_cmpType();
    switch (_type)
    {
    case 1:
        StepEnd--;
    case 0:
        re = (StepEnd - StepBegin + StepNum + 1) / StepNum;
        break;
    case 3:
        StepEnd++;
    case 2:
        re = (StepEnd - StepBegin + StepNum - 1) / StepNum;
        break;
    default:
        assert(0);
        break;
    }
    printf("re = %d\n", re);
    return re ? re : -1;
}

static inline bool _Strategy(Loop *loop, int times, int &codesize)
{
    if (times > 100 || times == 1)
        return false;
    int cnt = 0;
    for (BasicBlock *BB : *loop->get_BBs())
        cnt += BB->get_phinodes()->size() + BB->get_instrs()->size();
    cnt *= times;

    if (cnt > 2000)
        return false;
    else if (cnt > 1000)
    {
        if (codesize < 1000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (cnt > 500)
    {
        if (codesize < 2000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (cnt > 200)
    {
        if (codesize < 5000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (codesize <= 8000)
    {
        codesize += cnt;
        return true;
    }
    return false;
}

bool loopFullunroll::Finder(Loop *loop, Function *func)
{
    bool flag = false;
    for (Loop *son : *loop->get_lpsons())
    {
        flag = flag | Finder(son, func);
    }
    if (flag == true)
        return true;
    if (loop->get_exitings()->size() != 1 || loop->get_latchs()->size() != 1)
        return false;
    if (*loop->get_exitings()->begin() != *loop->get_latchs()->begin() || !loop->is_stepSet())
        return false;
    if (_checkLoop(loop)) //???
    {
    }
    int times = _checkFull(loop, _SCEV);
    if (times != -1 && _Strategy(loop, times, codesize))
    {
        FullUnroll(loop, times, func);
        return true;
    }

    return false;
}

void loopFullunroll::FuncDealer(Function *p_func)
{
    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    LCSSA worker(p_func, _Loop);
    worker.run();
    bool flag = false;
    for (Loop *son : *loop->get_lpsons())
    {
        if (flag)
            break;
        flag = flag | Finder(son, p_func);
    }
}

void loopFullunroll::PassRun(Module *p_module)
{
    puts("\n        Full  Unrolling        \n");
    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    codesize = 0;
    for (Function *p_func : *p_module->get_funcs())
        codesize += p_func->get_Values()->size() - 50;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}

loopFullunroll::loopFullunroll()
{
    _SCEV = new SCEV();
}

loopFullunroll::~loopFullunroll()
{
    delete _SCEV;
}