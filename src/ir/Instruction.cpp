#include <ir/Instrution.hpp>

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type)
    : User(basic_type), parent(_BB), instr_type(type)
{
}

Store::Store(Value *_addr, Value *_src, BasicBlock *_BB)
    : Instrution(_BB, InstrutionEnum::Store, TypeEnum::Void), p_addr(_addr), p_src(_src)
{
    Edge *p_in1 = new Edge(this, _addr);
    Edge *p_in2 = new Edge(this, _src);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
}