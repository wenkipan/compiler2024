#include "ir_opt/SSARegisterAlloc.hpp"
#include <backend/rv/RV.hpp>

#include <ir/ir.hpp>

class RVGen
{
    RVModule *rvm;
    std::unordered_map<Function * f, SSARegisterAlloc> ssara_map;

public:
    RVGen(/* args */);
    void run(Module *m);
    void init(Module *m);
    ~RVGen();
};
