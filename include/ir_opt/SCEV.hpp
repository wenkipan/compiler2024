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
    std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims;

public:
    SCEVEXP();
    ~SCEVEXP();

    void set_mul(bool _flag) { _mul = _flag; }
    void print();

    bool is_mul() { return _mul; }
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