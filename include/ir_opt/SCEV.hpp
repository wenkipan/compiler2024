#pragma once
#include <unordered_map>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Loop.hpp"

enum class SCEVType
{
    ADD,
    SUB,
    MUL,
    EXPADD0,
    EXPSUB0,
    EXPADD1,
    EXPSUB1,
    EXPADD2,
    EXPSUB2,
};

class SCEVEXP
{

    bool _mul = false;
    bool _ifmod = false;
    Value *ToPhi = nullptr;
    Value *_mod = nullptr;
    std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims;

public:
    static std::unordered_map<Value *, SCEVEXP *> *SCEVMAP;
    SCEVEXP();
    ~SCEVEXP();

    Value *get_scr(int i, BasicBlock *_BB);

    void set_mul(bool _flag) { _mul = _flag; }
    void set_ToPhi(Value *_val) { ToPhi = _val; }
    void set_mod(Value *_val) { _ifmod = true, _mod = _val; }
    void print();

    bool is_mul() { return _mul; }
    bool is_mod() { return _ifmod; }
    Value *get_ToPhi() { return ToPhi; }
    Value *get_mod() { return _mod; }
    Value *_getExpVal(SCEVEXP *val, int i, BasicBlock *_BB);
    std::vector<std::vector<std::pair<Value *, SCEVType>>> *get_dims() { return dims; }
};

class SCEV
{
public:
    std::unordered_map<Value *, SCEVEXP *> *SCEVMAP;
    Loop_Analysis *_Loop = nullptr;

    void SetStep();
    void PassRun(Module *_module);
    void LoopSCEVGen(Loop *lproot);
    void LoopSetStep(Loop *loop);

    std::unordered_map<Value *, SCEVEXP *> *get_map() { return SCEVMAP; }
    SCEVEXP *find_exp(Value *_val);

    void print();

    SCEV();
    ~SCEV();
};