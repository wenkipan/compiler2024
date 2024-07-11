#include <backend/rv/RVstand.hpp>
#include <vector>
#include <string>
class RVValue
{
};
class RVOperand : public RVValue
{
};
class RVAddr : public RVOperand
{
    RVValue *label;

public:
    RVAddr(RVValue *l) : label(l) {}
};
class RVReg : public RVOperand
{
    int rno;
    int offset;
    int is_offset = 0;

public:
    RVReg(int r) : rno(r) {}
    RVReg(int r, int off) : rno(r), offset(off), is_offset(1) {}
};
class RVImme : public RVOperand
{
    int imme;

public:
    RVImme(int i) : imme(i) {}
};
class RVInstr
{
    RVENUM instr_enum;
    RVextend extend_enum;
    RVcond cond_enum;
    std::vector<RVfloat> float_enums;

public:
    RVInstr(RVENUM i) : instr_enum(i){};
    RVInstr(RVENUM i, RVextend e) : instr_enum(i), extend_enum(e){};
    RVInstr(RVENUM i, RVcond c) : instr_enum(i), cond_enum(c){};
    RVInstr(RVENUM i, RVcond c, RVfloat f);
    RVInstr(RVENUM i, RVcond c, RVfloat f1, RVfloat f2);
};
class RVBlock : public RVValue
{
    std::string name;

public:
    RVBlock(std::string nam) : name(nam) {}
};
class RVFunc : public RVValue
{
    std::string name;

public:
    RVFunc(std::string nam) : name(nam) {}
};
class RVGlobalVariable : public RVValue
{
    std::string name;

public:
    RVGlobalVariable(std::string nam) : name(nam) {}
};
class RVModule
{
    std::vector<RVFunc *> funcs;
    std::vector<RVGlobalVariable *> globals;

public:
    void funcs_push_back(RVFunc *f) { funcs.push_back(f); }
    void globals_push_back(RVGlobalVariable *g) { globals.push_back(g); }
};