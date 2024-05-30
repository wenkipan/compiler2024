#pragma once
#include <ir/ir.hpp>
#include <ir_opt/Loop.hpp>
#include <unordered_map>

enum class SCEVType
{
    ADD,
    SUB,
    MUL,
};

class SCEVEXP
{
    bool _mul = false;
    bool _ifmod = false;
    Value *_mod = nullptr;
    std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims;

public:
    SCEVEXP();
    ~SCEVEXP();

    Value *get_scr(int i, BasicBlock *_BB);

    void set_mul(bool _flag) { _mul = _flag; }
    void set_mod(Value *_val) { _ifmod = true, _mod = _val; }
    void print();

    bool is_mul() { return _mul; }
    bool is_mod() { return _ifmod; }
    Value *get_mod() { return _mod; }
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