
#include "../../include/ir/Value.hpp"
#include "../../include/ir_opt/IRCopy.hpp"
#include "../../include/util/RPO.hpp"
#include "ir/Type.hpp"

Value *IRCopy::get_mapval(Value *a)
{
    if (is_a<GlobalValue>(a))
        return a;
    if (instrsmap.find(a) == instrsmap.end())
    {
        if (is_a<ConstantI32>(a))
        {
            ConstantI32 *c = new ConstantI32((ConstantI32 *)a);
            dealingf->value_pushBack(c);
            instrsmap.emplace(a, c);
            return c;
        }
        else if (is_a<ConstantF32>(a))
        {
            ConstantF32 *c = new ConstantF32((ConstantF32 *)a);
            dealingf->value_pushBack(c);
            instrsmap.emplace(a, c);
            return c;
        }
        else
        {
            Value *deadvalue = new Value;
            instrsmap.emplace(a, deadvalue);
            return deadvalue;
        }
    }
    return instrsmap.find(a)->second;
}
BasicBlock *IRCopy::get_mapbb(BasicBlock *a)
{
    assert(bbsmap.find(a) != bbsmap.end());
    return bbsmap.find(a)->second;
}
void IRCopy::emplace_val(Value *a, Value *b)
{
    auto it = instrsmap.find(a);
    if (it != instrsmap.end())
    {
        for (auto useredge : *it->second->get_user_list())
        {
            useredge->set_val(b);
        }
        it->second->get_user_list()->clear();
        delete it->second;
        instrsmap.erase(it);
    }
    instrsmap.emplace(a, b);
}
Function *IRCopy::copy_func(Function *f)
{
    assert(!f->get_isExternal());
    Function *newfunc = new Function(f->get_name() + "_copy");
    dealingf = newfunc;
    // params
    for (auto param : *f->get_params())
    {
        Param *p = new Param(param->get_type(), false);
        newfunc->Param_pushBack(p);
        newfunc->value_pushBack(p);
        emplace_val(param, p);
    }
    // blocks
    for (auto BB : *f->get_blocks())
    {
        BasicBlock *newBB = new BasicBlock(newfunc);
        bbsmap.emplace(BB, newBB);
        newfunc->block_pushBack(newBB);
        if (BB == f->get_entryBB())
        {
            newfunc->set_entryBB(newBB);
        }
        if (BB == f->get_retBB())
        {
            newfunc->set_retBB(newBB);
        }
    }

    for (auto BB : RPO(f))
    {
        BasicBlock *newBB = get_mapbb(BB);
        // phis
        for (auto phi : *BB->get_phis())
        {
            PHINode *p = new PHINode(newBB, phi->get_type()->get_type(), true);
            newBB->Insert_Phi(p);
            for (auto kv : *phi->get_valueMap())
            {
                p->addIncoming(get_mapval(kv.second->get_val()), get_mapbb(kv.first));
            }
            emplace_val(phi, p);
        }
        // instrs
        for (auto instr : *BB->get_instrs())
        {
            Value *newinstr = nullptr;
            Value *a = nullptr;
            Value *b = nullptr;
            if (instr->isAlloca())
                newinstr = new Alloca(newBB, instr->get_type());
            else if (instr->isGEP())
            {
                a = get_mapval(((GEP *)instr)->get_addr());
                b = get_mapval(((GEP *)instr)->get_offset());
                newinstr = new GEP(a, b, ((GEP *)instr)->get_isele(), newBB);
            }
            else if (instr->isReturn())
            {
                if (((Ret *)instr)->get_type()->get_type() == TypeEnum::Void)
                {
                    newinstr = new Ret(newBB);
                }
                else
                {
                    newinstr = new Ret(get_mapval(((Ret *)instr)->get_val()), newBB);
                }
            }
            else if (instr->isJmp())
                newinstr = new Jmp(get_mapbb(((Jmp *)instr)->get_nextBB()), newBB);
            else if (instr->isBranch())
                newinstr = new Branch(get_mapval(((Branch *)instr)->get_cond()),
                                      get_mapbb(((Branch *)instr)->get_trueBB()),
                                      get_mapbb(((Branch *)instr)->get_falseBB()), newBB);
            else if (instr->isLoad())
            {

                newinstr = new Load(get_mapval(((Load *)instr)->get_addr()),
                                    ((Load *)instr)->get_is_stack_prt(), newBB);
                if (instr->isVecI32type())
                    newinstr->get_type()->reset(TypeEnum::VecI32);
            }
            else if (instr->isStore())
                newinstr = new Store(get_mapval(((Store *)instr)->get_addr()),
                                     get_mapval(((Store *)instr)->get_src()),
                                     ((Store *)instr)->get_isptr(), newBB);
            else if (instr->isCmp())
                newinstr = new Cmp(((Cmp *)instr)->get_Instrtype(),
                                   get_mapval(((Cmp *)instr)->get_src1()),
                                   get_mapval(((Cmp *)instr)->get_src2()), newBB);
            else if (instr->isBinary())
            {

                newinstr = new Binary(((Binary *)instr)->get_Instrtype(),
                                      get_mapval(((Binary *)instr)->get_src1()),
                                      get_mapval(((Binary *)instr)->get_src2()), newBB);
                if (instr->isVecI32type())
                    newinstr->get_type()->reset(TypeEnum::VecI32);
            }
            else if (instr->get_Instrtype() == InstrutionEnum::Assign)
            {
                newinstr = new Assign(InstrutionEnum::Assign, get_mapval(((Unary *)instr)->get_src()), newBB, false);
                if (instr->isVecI32type())
                    newinstr->get_type()->reset(TypeEnum::VecI32);
            }
            else if (instr->isUnary())
                newinstr = new Unary(((Unary *)instr)->get_Instrtype(),
                                     get_mapval(((Unary *)instr)->get_src()), newBB);
            else if (instr->isCall())
            {
                newinstr = new Call(((Call *)instr)->get_func(), newBB); // not change!!
                for (auto it = instr->get_value_list()->begin() + 1; it != instr->get_value_list()->end(); it++)
                {
                    ((Call *)newinstr)->params_pushback(get_mapval((*it)->get_val()));
                }
            }
            else
                assert(0);
            assert(newinstr);
            emplace_val(instr, newinstr);
        }
    }
    if (0)
    {
        for (auto param : *newfunc->get_params())
            param->print();
        printf("\n");
        for (auto bb : *newfunc->get_blocks())
        {
            bb->print();
        }
        newfunc->get_retBB()->print();
        puts("fdones");
    }
    dealingf = nullptr;
    return newfunc;
}
