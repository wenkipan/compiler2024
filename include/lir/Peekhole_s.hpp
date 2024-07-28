#include "../../include/ir/ir.hpp"

class Peekhole_s
{
public:
    void toSHL(Function *f);
    void toASHR(Function *f);
    void run(Function *f);
};