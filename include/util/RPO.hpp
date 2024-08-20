#include "../../include/ir/ir.hpp"

std::vector<BasicBlock *> RPO(Function *f);

#define CHECK1(val, num)                                          \
    {                                                             \
        ConstantI32 *constNum = dynamic_cast<ConstantI32 *>(val); \
        if (constNum == nullptr)                                  \
            return;                                               \
        if (constNum->get_32_at(0) != num)                        \
            return;                                               \
    }

#define CHECK2(val, type)                                    \
    {                                                        \
        Instrution *instr = dynamic_cast<Instrution *>(val); \
        if (instr == nullptr)                                \
            return;                                          \
        if (instr->get_Instrtype() != type)                  \
            return;                                          \
    }

#define CHECK3(val1, val2) \
    {                      \
        if (val1 != val2)  \
            return;        \
    }