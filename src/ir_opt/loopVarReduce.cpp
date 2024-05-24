#include <ir_opt/loopVarReduce.hpp>

void loopVarReduce::Reduce(Loop *loop, DomTree &_domtree)
{
    for (Loop *it : *loop->get_lpsons())
        Reduce(it, _domtree);
    if (loop->get_header() == nullptr)
        return;
    if (loop->get_latchs()->size() != 1)
        return;
    BasicBlock *prevBB = loop->get_prev();
    BasicBlock *_latch = (*loop->get_latchs()->begin());
    for (BasicBlock *_BB : *loop->get_nwBBs())
    {
        if (!_domtree.is_dom(_BB, _latch))
            continue;
        std::vector<Instrution *> *instrs = _BB->get_instrs();
        SCEVEXP *p_exp = nullptr;
        int _size = instrs->size();
        for (int i = 0; i < _size; ++i)
        {
            if ((p_exp = _SCEV->find_exp((*instrs)[i])) == nullptr || !p_exp->is_mul())
                continue;
            std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = p_exp->get_dims();
            if (!(*dims)[2].empty() || !(*dims)[3].empty() || !(*dims)[4].empty())
                continue;
            assert(!(*dims)[0].empty());
            Value *src1 = nullptr;
            int flag = 0;
            if ((*dims)[0][0].first->get_type()->get_type() == TypeEnum::I32)
                src1 = new ConstantI32(0), flag = 0;
            else if ((*dims)[0][0].first->get_type()->get_type() == TypeEnum::F32)
                src1 = new ConstantF32(0.0), flag = 5;
            else
                assert(0);
            _BB->get_func()->value_pushBack(src1);
            Instrution *p_branch = nullptr;
            Instrution *p_instr = nullptr;
            for (auto &i : (*dims)[0])
            {

                switch (i.second)
                {
                case SCEVType::ADD:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(24 + flag), src1, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                case SCEVType::SUB:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(25 + flag), src1, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                case SCEVType::MUL:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(26 + flag), src1, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                default:
                    break;
                }
                src1 = (Value *)p_instr;
            }
            Value *src2 = nullptr;
            assert((*dims)[0][0].first->get_type()->get_type() == (*dims)[1][0].first->get_type()->get_type());
            if ((*dims)[1][0].first->get_type()->get_type() == TypeEnum::I32)
                src2 = new ConstantI32(0);
            else if ((*dims)[1][0].first->get_type()->get_type() == TypeEnum::F32)
                src2 = new ConstantF32(0.0);
            else
                assert(0);
            _BB->get_func()->value_pushBack(src2);
            for (auto &i : (*dims)[1])
            {

                switch (i.second)
                {
                case SCEVType::ADD:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(24 + flag), src2, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                case SCEVType::SUB:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(25 + flag), src2, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                case SCEVType::MUL:
                    p_branch = prevBB->get_last_instrution();
                    prevBB->get_instrs()->pop_back();
                    p_instr = new Binary((InstrutionEnum)(26 + flag), src2, i.first, prevBB);
                    prevBB->get_instrs()->emplace_back(p_branch);
                    break;
                default:
                    break;
                }
                src2 = (Value *)p_instr;
            }
            PHINode *_phi = new PHINode(loop->get_header(), src1->get_type()->get_type(), true);
            _BB->Insert_Phi(_phi);
            (*instrs)[i]->replaceAllUses(_phi);
            _phi->addIncoming(src1, prevBB);
            p_branch = _latch->get_last_instrution();
            _latch->get_instrs()->pop_back();
            p_instr = new Binary((InstrutionEnum)(24 + flag), _phi, src2, _latch);
            _latch->get_instrs()->emplace_back(p_branch);
            _phi->addIncoming(p_instr, _latch);
        }
    }
}

void loopVarReduce::PassRun(Module *_module)
{
    _SCEV->PassRun(_module);
    _Loop = _SCEV->_Loop;
    for (Function *p_func : *_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        DomTree _domtree(p_func);
        _domtree.Run();
        Reduce(_Loop->get_LoopInfo()->find(p_func)->second, _domtree);
    }
}

loopVarReduce::loopVarReduce()
{
    _SCEV = new SCEV();
}

loopVarReduce::~loopVarReduce()
{
    delete _SCEV;
}