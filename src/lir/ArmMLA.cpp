#include "../../include/lir/ARMMLA.hpp"
#include "ir/Function.hpp"

void ARMMLA::int_mla(Function *f)
{
    std::vector<Instrution *> work;
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->get_Instrtype() == InstrutionEnum::IMUL || i->get_Instrtype() == InstrutionEnum::SHL)
                if (i->get_user_list()->size() == 1 && (i->get_type()->get_type() == TypeEnum::I32 || i->get_type()->get_type() == TypeEnum::Ptr))
                {
                    assert(is_a<Instrution>(i->get_user_list()->at(0)->get_user()));
                    Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
                    if (use->get_Instrtype() == InstrutionEnum::IADD)
                    {
                        if (i == use->get_operand_at(0) && !is_a<ConstantI32>(use->get_operand_at(1)))
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                        else if (i == use->get_operand_at(1) && !is_a<ConstantI32>(use->get_operand_at(0)))
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                    }
                    else if (use->get_Instrtype() == InstrutionEnum::ISUB)
                    {
                        if (i == use->get_operand_at(1) && !is_a<ConstantI32>(use->get_operand_at(0)))
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                    }
                }

    for (auto i : work)
    {
        Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
        if (use->get_Instrtype() == InstrutionEnum::IADD || use->get_Instrtype() == InstrutionEnum::ISUB)
        {
        }
        else
            continue;
        Value *a;
        Value *b = i->get_operand_at(0);
        Value *c = i->get_operand_at(1);
        if (i == use->get_operand_at(0))
            a = use->get_operand_at(1);
        else if (i == use->get_operand_at(1))
            a = use->get_operand_at(0);
        else
            assert(0);
        BasicBlock *BB = use->get_parent();

        // i->print();
        // use->print();

        // a->print();
        // b->print();
        // c->print();
        Instrution *newi = nullptr;
        if (i->get_Instrtype() == InstrutionEnum::IMUL)
        {
            if (use->get_Instrtype() == InstrutionEnum::IADD)
                newi = new Triple(InstrutionEnum::MLA, a, b, c, BB);
            else if (use->get_Instrtype() == InstrutionEnum::ISUB)
                newi = new Triple(InstrutionEnum::MLS, a, b, c, BB);
        }
        else if (i->get_Instrtype() == InstrutionEnum::SHL)
        {
            if (use->get_Instrtype() == InstrutionEnum::IADD)
                newi = new Triple(InstrutionEnum::ADDlsl, a, b, c, BB);
            else if (use->get_Instrtype() == InstrutionEnum::ISUB)
                newi = new Triple(InstrutionEnum::SUBlsl, a, b, c, BB);
        }
        else
            assert(0);

        // Ptr *fake = new Ptr(newi->get_type());
        // delete newi->get_type();
        // newi->set_type(fake);
        // printf("test   ");
        // newi->print();
        // printf("\n");
        BB->instr_insert_before(use, newi);
        use->replaceAllUses(newi);

        use->drop();
        i->drop();
    }
}

void ARMMLA::run(Function *f)
{
    int_mla(f);
    float_mla(f);
    // vec_mla(f);
}

void ARMMLA::float_mla(Function *f)
{
    std::vector<Instrution *> work;
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->get_Instrtype() == InstrutionEnum::FMUL)
                if (i->get_user_list()->size() == 1)
                {
                    assert(is_a<Instrution>(i->get_user_list()->at(0)->get_user()));
                    Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
                    if (use->get_Instrtype() == InstrutionEnum::FADD)
                    {
                        if (i == use->get_operand_at(0) && use->get_operand_at(1)->get_user_list()->size() == 1)
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                        else if (i == use->get_operand_at(1) && use->get_operand_at(0)->get_user_list()->size() == 1)
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                    }
                }

    for (auto i : work)
    {
        Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
        if (use->get_Instrtype() != InstrutionEnum::FADD)
            continue;
        Value *a;
        Value *b = i->get_operand_at(0);
        Value *c = i->get_operand_at(1);
        if (i == use->get_operand_at(0))
            a = use->get_operand_at(1);
        else if (i == use->get_operand_at(1))
            a = use->get_operand_at(0);
        else
            assert(0);
        BasicBlock *BB = use->get_parent();

        // i->print();
        // use->print();

        // a->print();
        // b->print();
        // c->print();
        Instrution *newi = nullptr;
        if (i->get_Instrtype() == InstrutionEnum::FMUL)
            newi = new Triple(InstrutionEnum::FMLA, a, b, c, BB);
        else
            assert(0);

        // Ptr *fake = new Ptr(newi->get_type());
        // delete newi->get_type();
        // newi->set_type(fake);
        // printf("test   ");
        // newi->print();
        // printf("\n");
        BB->instr_insert_before(use, newi);
        use->replaceAllUses(newi);

        use->drop();
        i->drop();
    }
}
void ARMMLA::vec_mla(Function *f)
{
    std::vector<Instrution *> work;
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->get_Instrtype() == InstrutionEnum::VMLA)
                if (i->get_user_list()->size() == 1)
                {
                    assert(is_a<Instrution>(i->get_user_list()->at(0)->get_user()));
                    Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
                    if (use->get_Instrtype() == InstrutionEnum::IADD)
                    {
                        if (i == use->get_operand_at(0) && use->get_operand_at(1)->get_user_list()->size() == 1)
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                        else if (i == use->get_operand_at(1) && use->get_operand_at(0)->get_user_list()->size() == 1)
                        {
                            // printf("????   \n");
                            // i->print();
                            // use->print();
                            work.push_back(i);
                        }
                    }
                }

    for (auto i : work)
    {
        Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
        assert(use->get_Instrtype() == InstrutionEnum::IADD);

        Value *a;
        Value *b = i->get_operand_at(0);
        Value *c = i->get_operand_at(1);
        if (i == use->get_operand_at(0))
            a = use->get_operand_at(1);
        else if (i == use->get_operand_at(1))
            a = use->get_operand_at(0);
        else
            assert(0);
        BasicBlock *BB = use->get_parent();

        // i->print();
        // use->print();

        // a->print();
        // b->print();
        // c->print();
        Instrution *newi = nullptr;
        if (i->get_Instrtype() == InstrutionEnum::IMUL)
            newi = new Triple(InstrutionEnum::VMLA, a, b, c, BB);
        else
            assert(0);

        // Ptr *fake = new Ptr(newi->get_type());
        // delete newi->get_type();
        // newi->set_type(fake);
        // printf("test   ");
        // newi->print();
        // printf("\n");
        BB->instr_insert_before(use, newi);
        use->replaceAllUses(newi);

        use->drop();
        i->drop();
    }
}