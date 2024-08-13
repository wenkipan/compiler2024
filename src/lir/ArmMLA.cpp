#include "../../include/lir/ARMMLA.hpp"
#include "ir/Instrution.hpp"

void ARMMLA::run(Function *f)
{
    std::vector<Instrution *> work;
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->get_Instrtype() == InstrutionEnum::IMUL || i->get_Instrtype() == InstrutionEnum::SHL)
                if (i->get_user_list()->size() == 1)
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
                }

    for (auto i : work)
    {
        Instrution *use = (Instrution *)i->get_user_list()->at(0)->get_user();
        if (use->get_Instrtype() != InstrutionEnum::IADD)
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
            newi = new Triple(InstrutionEnum::MLA, a, b, c, BB);
        else if (i->get_Instrtype() == InstrutionEnum::SHL)
            newi = new Triple(InstrutionEnum::ADDlsl, a, b, c, BB);
        else
            assert(0);

        // Ptr *fake = new Ptr(newi->get_type());
        // delete newi->get_type();
        // newi->set_type(fake);
        printf("test   ");
        newi->print();
        printf("\n");
        BB->instr_insert_before(use, newi);
        use->replaceAllUses(newi);

        use->drop();
        i->drop();
    }
}