#include "../../include/lir/modTosubmul.hpp"

void modTosubmul::run(Function *f)
{
    int flag = 1;
    while (flag)
    {
        flag = 0;
        for (auto BB : *f->get_blocks())
        {
            int pos = 0;
            for (auto i : *BB->get_instrs())
            {
                if (i->get_Instrtype() == InstrutionEnum::IMOD)
                {
                    flag = 1;
                    Value *a = i->get_operand_at(0);
                    Value *b = i->get_operand_at(1);
                    // int q = a / b;
                    Instrution *q = new Binary(InstrutionEnum::IDIV, a, b, BB);
                    // int s=b*q;
                    Instrution *s = new Binary(InstrutionEnum::IMUL, b, q, BB);
                    // int r = a - s;
                    Instrution *r = new Binary(InstrutionEnum::ISUB, a, s, BB);
                    r->insertInstr(BB, pos);
                    s->insertInstr(BB, pos);
                    q->insertInstr(BB, pos);
                    i->replaceAllUses(r);
                    i->drop();
                    break;
                }
                pos++;
            }
        }
    }
}