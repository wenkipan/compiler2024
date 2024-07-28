#include "../../include/lir/modTofunc.hpp"

static inline Function *gen_fdiv(Module *m)
{
    auto fack = new Type(TypeEnum ::I32);
    auto newf = new Function(fack, "__aeabi_idivmod");
    newf->set_isExternal();
    auto p1 = new Param(fack, 0);
    newf->Param_pushBack(p1);
    newf->value_pushBack(p1);
    auto p2 = new Param(fack, 0);
    newf->Param_pushBack(p2);
    newf->value_pushBack(p2);
    m->func_push_back(newf);
    delete fack;
    return newf;
}
static inline int find_pos(BasicBlock *b, Instrution *i)
{
    int pos = 0;
    for (auto it : *b->get_instrutions())
    {
        if (it == i)
            return pos;
        pos++;
    }
    assert(0);
}
void modTofunc::run(Module *m)
{
    Function *newf = nullptr;
    auto fs = *m->get_funcs();
    for (auto f : fs)
    {
        if (f->get_isExternal())
            continue;
        for (auto B : *f->get_blocks())
        {
            auto is = *B->get_instrs();
            for (auto i : is)
                if (is_a<Binary>(i) && i->get_Instrtype() == InstrutionEnum::IMOD)
                {
                    if (!newf)
                        newf = gen_fdiv(m);

                    assert(i->get_value_list()->size() == 2);
                    Call *newc = new Call(newf, B);
                    newc->params_pushback(i->get_operand_at(0));
                    newc->params_pushback(i->get_operand_at(1));
                    i->replaceAllUses(newc);
                    newc->insertInstr(B, find_pos(B, i));
                    i->drop();
                }
        }
    }
}