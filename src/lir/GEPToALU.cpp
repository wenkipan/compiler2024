#include "ir/BasicBlock.hpp"
#include "ir_opt/DCE.hpp"
#include "ir_opt/SimplifyCFG.hpp"
#include <algorithm>
#include <iostream>
#include <lir/GEPToALU.hpp>
#include <ostream>

static inline std::vector<int> can_combine_gep(Instrution *i, std::vector<int> poss)
{
    printf("deal gep");
    i->print();
    if (!is_a<GEP>(i))
        return poss;
    if (i->get_user_list()->size() == 1 && is_a<ConstantI32>(i->get_operand_at(1)) && ((GEP *)i)->get_isele())
    {
        int pos = ((ConstantI32 *)i->get_operand_at(1))->get_32_at(0);
        poss.push_back(pos);
    }
    else
        poss.clear();
    return can_combine_gep((Instrution *)i->get_user_list()->at(0)->get_user(), poss);
}
void GEPToALU::combine_gep(Function *f)
{
    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
        {
            if (instr->isGEP() && (is_a<Alloca>(instr->get_operand_at(0)) || is_a<GlobalVariable>(instr->get_operand_at(0))))
            {
                std::vector<int> poss;
                poss = can_combine_gep(instr, poss);
                // combine
                if (poss.size() > 1)
                {
                    for (auto i : poss)
                    {
                        std::cout << i;
                    }
                    // assert(poss.size() == ((ArrayType *)((Ptr *)instr->get_operand_at(0)->get_type())->get_btype())->get_dims()->size());
                    Value *lastgep = instr;
                    for (int i = 0; i < poss.size() - 1; i++)
                        lastgep = lastgep->get_user_list()->at(0)->get_user();

                    assert(lastgep != instr);
                    // change ptr
                    Value *pre = ((Instrution *)lastgep)->get_operand_at(0);
                    auto it = std::find(pre->get_user_list()->begin(), pre->get_user_list()->end(), lastgep->get_value_list()->at(0));
                    pre->get_user_list()->erase(it);
                    lastgep->get_value_list()->at(0)->set_val(instr->get_operand_at(0));
                    lastgep->get_value_list()->at(1)->drop();
                    // change gep
                    for (auto pos : poss)
                    {
                        ConstantI32 *newi = new ConstantI32(pos);
                        f->value_pushBack(newi);
                        new Edge(lastgep, newi);
                    }
                }
            }
        }
    }
    f->print();
    DCE dce;
    dce.run(f);
    SimplifyCFG cfg;
    cfg.run(f);
}
void GEPToALU::run(Function *f)
{
    combine_gep(f);
    return;
    std::unordered_map<Instrution *, std::vector<Instrution *>> replacemap;
    for (auto BB : *f->get_blocks())
    {
        auto origininstrs = *BB->get_instrs();
        for (auto instr : origininstrs)
        {
            std::vector<Instrution *> newinstrs;
            if (instr->isGEP())
            {
                Value *gepsrc = instr->get_operand_at(0);
                Type *pointeety = ((Ptr *)gepsrc->get_type())->get_btype();
                TypeEnum te;
                int maxsize = 1;
                std::vector<int> dims;
                if (pointeety->get_type() == TypeEnum::Array)
                {
                    dims = *((ArrayType *)pointeety)->get_dims();
                    maxsize = ((ArrayType *)pointeety)->get_size();
                    if (((GEP *)instr)->get_isele())
                        maxsize /= dims.at(0);
                    te = ((ArrayType *)pointeety)->get_basic_type();
                }
                else
                {
                    assert(pointeety->get_type() == TypeEnum::I32 || pointeety->get_type() == TypeEnum::F32);
                    te = pointeety->get_type();
                    dims.push_back(1);
                }
                int size;
                if (te == TypeEnum::I32 || te == TypeEnum ::F32)
                    size = 4;
                else
                    assert(0);

                int offset = 0;
                Value *src2 = nullptr;
                for (int i = 0; i < instr->get_value_list()->size() - 1; i++) // first val is ptr
                {
                    Value *vi = instr->get_value_list()->at(i + 1)->get_val();
                    if (auto c = dynamic_cast<ConstantI32 *>(vi))
                    {
                        offset += maxsize * c->get_32_at(0) * size;
                    }
                    else
                    {
                        ConstantI32 *i32 = new ConstantI32(size * maxsize);
                        f->value_pushBack(i32);
                        Instrution *newmuli = new Binary(InstrutionEnum::IMUL, vi, i32, BB);
                        newinstrs.push_back(newmuli);
                        if (src2)
                        {
                            src2 = new Binary(InstrutionEnum::IADD, src2, newmuli, BB);
                            newinstrs.push_back((Instrution *)src2);
                        }
                        else
                            src2 = newmuli;
                    }
                    maxsize /= dims.at(i);
                }
                ConstantI32 *ioff = new ConstantI32(offset);
                f->value_pushBack(ioff);
                if (src2)
                {
                    src2 = new Binary(InstrutionEnum::IADD, ioff, src2, BB);
                    newinstrs.push_back((Instrution *)src2);
                }
                else
                    src2 = ioff;

                // create final for load or another gep
                Instrution *final = new Binary(InstrutionEnum::IADD, instr->get_operand_at(0), src2, BB);
                Ptr *fake = new Ptr(final->get_type());
                delete final->get_type();
                final->set_type(fake);
                newinstrs.push_back(final);

                replacemap.emplace(instr, newinstrs);
            }
        }
    }

    // replace
    for (auto kv : replacemap)
    {
        auto newinstrs = kv.second;
        auto instr = kv.first;
        auto final = newinstrs.back();
        BasicBlock *BB = instr->get_parent();
        // replace
        instr->replaceAllUses(final);
        // move to right place
        int insertpos = 0;
        for (; insertpos < BB->get_instrs()->size(); insertpos++)
            if (BB->get_instrs()->at(insertpos) == instr)
                break;
        for (auto instrc : newinstrs)
        {
            instrc->insertInstr(BB, insertpos++);
        }
        instr->drop();
    }
}