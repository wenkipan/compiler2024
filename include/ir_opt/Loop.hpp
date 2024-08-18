#pragma once
#include <set>

#include "../../include/ir/ir.hpp"

class NaLoop
{
    BasicBlock *entryBB;
    std::set<BasicBlock *> *BBs;

public:
    NaLoop(BasicBlock *_entry);
    ~NaLoop();

    BasicBlock *get_entryBB() { return entryBB; }
    std::set<BasicBlock *> *get_BBs() { return BBs; };

    bool is_InBBs(BasicBlock *_BB);
    void addBB(BasicBlock *_BB) { BBs->insert(_BB); }
};

class Loop
{
    Loop *parent = nullptr;
    int lpDepth = 0;
    std::set<Loop *> *lpsons;

    BasicBlock *header = nullptr;
    BasicBlock *prevHeader = nullptr;
    std::set<BasicBlock *> *BBs;
    std::set<BasicBlock *> *nwBBs;
    std::set<BasicBlock *> *enters;
    std::set<BasicBlock *> *latchs;
    std::set<BasicBlock *> *exitings;
    std::set<BasicBlock *> *exits;

    bool hasCall_ = false;

    // step
    bool _is_stepSet = false;
    int _cmpType;
    Value *lpStep = nullptr;
    Value *lpEnd = nullptr;
    Value *lpCmp = nullptr;
    Value *_step1 = nullptr;
    Value *_step2 = nullptr;

public:
    Loop();
    ~Loop();

    void createPrevHeader(Loop *loop);

    void set_prevHead(BasicBlock *_BB) { prevHeader = _BB; }
    void set_depth(int _d) { lpDepth = _d; }
    void set_parent(Loop *_fa) { parent = _fa; }
    void set_ifStep(bool _val) { _is_stepSet = _val; }
    void set_cmpType(int _x) { _cmpType = _x; }
    void set_lpStep(Value *_val) { lpStep = _val; }
    void set_lpEnd(Value *_val) { lpEnd = _val; }
    void set_lpCmp(Value *_val) { lpCmp = _val; }
    void set_calSetp(Value *_val1, Value *_val2) { _step1 = _val1, _step2 = _val2; }

    Loop *get_parent() { return parent; }
    int get_lpDepth() { return lpDepth; }
    std::set<Loop *> *get_lpsons() { return lpsons; }
    bool is_simple();
    bool is_BBinLoop(BasicBlock *_BB);

    void orhasCall(bool flag) { hasCall_ = flag ? true : hasCall_; }
    bool hasCall() { return hasCall_; }

    void set_header(BasicBlock *_header) { header = _header; }

    BasicBlock *get_header() { return header; }
    BasicBlock *get_prev() { return prevHeader; }
    std::set<BasicBlock *> *get_BBs() { return BBs; }
    std::set<BasicBlock *> *get_nwBBs() { return nwBBs; }
    std::set<BasicBlock *> *get_enters() { return enters; }
    std::set<BasicBlock *> *get_latchs() { return latchs; }
    std::set<BasicBlock *> *get_exitings() { return exitings; }
    std::set<BasicBlock *> *get_exits() { return exits; }

    bool is_stepSet() { return _is_stepSet; }
    int get_cmpType() { return _cmpType; }
    Value *get_lpStep() { return lpStep; }
    Value *get_lpEnd() { return lpEnd; }
    Value *get_lpCmp() { return lpCmp; }
    Value *get_calStep(bool _flag) { return _flag ? _step2 : _step1; }
};

class LoopNode
{
    Loop *fa = nullptr;
    int lpdepth = 0;

    bool isHead = false;
    bool isExit = false;
    bool isEntry = false;
    bool isLatch = false;
    bool isExiting = false;

public:
    void add_depth() { lpdepth++; }
    void set_depth(int _d) { lpdepth = _d; }
    void set_loop(Loop *_loop) { fa = _loop; }

    Loop *get_loop() { return fa; }
    int get_depth() { return lpdepth; }
    bool is_head() { return isHead; }
    bool is_exit() { return isExit; }
    bool is_entry() { return isEntry; }
    bool is_latch() { return isLatch; }
    bool is_exiting() { return isExiting; }
};

class Loop_Analysis
{
    std::unordered_map<Function *, Loop *> *LoopInfo;
    std::unordered_map<BasicBlock *, LoopNode *> *BBmap;

public:
    Loop_Analysis();
    ~Loop_Analysis();

    void loop_addprevBB(Loop *nwloop);
    void loop_BBsAdd(Loop *nwloop);
    void buildnest(Loop *nwloop, int nwlevel, std::unordered_map<BasicBlock *, Loop *> &_map);
    void FuncAnalysis(Function *p_func);
    void PassRun(Module *p_module);

    std::unordered_map<Function *, Loop *> *get_LoopInfo() { return LoopInfo; }
    std::unordered_map<BasicBlock *, LoopNode *> *get_BBmap() { return BBmap; }
};