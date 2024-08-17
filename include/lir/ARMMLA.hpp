#include "../../include/ir/ir.hpp"

class ARMMLA
{
public:
    void int_mla(Function *f);
    void float_mla(Function *f);
    void vec_mla(Function *f);
    void run(Function *f);
};
