#include "../../include/ir_opt/loopVarReduce.hpp"

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
        for (int i = 0; i < instrs->size(); ++i)
        {
            if ((p_exp = _SCEV->find_exp((*instrs)[i])) == nullptr || !p_exp->is_mul())
                continue;
            std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = p_exp->get_dims();
            if (!(*dims)[2].empty() || !(*dims)[3].empty() || !(*dims)[4].empty())
                continue;
            assert(!(*dims)[0].empty());
            Instrution *p_branch = prevBB->get_last_instrution();
            prevBB->Ins_popBack();
            Value *src1 = p_exp->get_scr(0, prevBB);
            Value *src2 = p_exp->get_scr(1, prevBB);

            assert(src1->get_type()->get_type() == src2->get_type()->get_type());
            int flag = 0;
            if (src1->get_type()->get_type() == TypeEnum::F32)
                flag = 5;
            src1 = new Binary((InstrutionEnum)(25 + flag), src1, src2, prevBB);
            prevBB->Ins_pushBack(p_branch);
            PHINode *_phi = new PHINode(loop->get_header(), src1->get_type()->get_type(), true);
            loop->get_header()->Insert_Phi(_phi);
            _phi->addIncoming(src1, prevBB);
            p_branch = _latch->get_last_instrution();
            _latch->get_instrs()->pop_back();
            Instrution *p_instr = new Binary((InstrutionEnum)(24 + flag), _phi, src2, _latch);
            p_instr->insertInstr(loop->get_header(), 0);
            if (_BB == loop->get_header())
                ++i;
            if (p_exp->is_mod())
            {
                p_instr = new Binary(InstrutionEnum::IMOD, p_instr, p_exp->get_mod(), _latch);
                p_instr->insertInstr(loop->get_header(), 1);
                if (_BB == loop->get_header())
                    ++i;
            }
            (*instrs)[i]->replaceAllUses(p_instr);
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