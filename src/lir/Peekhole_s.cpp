#include "ir/Constant.hpp"
#include "ir/Instrution.hpp"
#include <lir/Peekhole_s.hpp>
static inline int to2pow(int n)
{
    if (n > 0 && (n & (n - 1)) == 0)
    {
        int exponent = 0;
        while (n > 1)
        {
            n >>= 1; // 等价于 n = n / 2
            exponent++;
        }
        return exponent;
    }
    else
        return 0;
}
// void Peekhole_s::toSHL(Function *f)
// {
//     Value *op1 = ;
// }

void Peekhole_s::run(Function *f)
{
    for (auto B : *f->get_blocks())
    {
        auto fackinstrs = *B->get_instrs();
        for (auto i : fackinstrs)
        {
            if (i->get_Instrtype() == InstrutionEnum::IMUL)
            {
                Value *op1 = i->get_operand_at(0);
                Value *op2 = i->get_operand_at(1);
                assert(!(is_a<ConstantI32>(op1) && is_a<ConstantI32>(op2)));
                if (is_a<ConstantI32>(op1))
                    std::swap(op1, op2);
                if (is_a<ConstantI32>(op2) && ((ConstantI32 *)op2)->get_32_at(0))
                {
                    int exp = to2pow(((ConstantI32 *)op2)->get_32_at(0));
                    if (exp)
                    {
                        auto newcon = new ConstantI32(exp);
                        f->value_pushBack(newcon);
                        auto newshl = new Binary(InstrutionEnum::SHL, op1, newcon, B);
                        printf("___________newinst\n");
                        newshl->print();
                        B->instr_insert_before(i, newshl);
                        i->replaceAllUses(newshl);
                        i->drop();
                    }
                }
            }
            // else if (i->get_Instrtype() == InstrutionEnum::IDIV && is_a<ConstantI32>(i->get_operand_at(1)))
            // {
            //     Value *op1 = i->get_operand_at(0);
            //     Value *op2 = i->get_operand_at(1);
            //     int exp = to2pow(((ConstantI32 *)op2)->get_32_at(0));
            //     if (exp)
            //     {
            //         auto newcon = new ConstantI32(exp);
            //         f->value_pushBack(newcon);
            //         auto newshl = new Binary(InstrutionEnum::ASHR, op1, newcon, B);
            //         printf("___________newinst222\n");
            //         newshl->print();
            //         B->instr_insert_before(i, newshl);
            //         i->replaceAllUses(newshl);
            //         i->drop();
            //     }
            // }
        }
    }
}