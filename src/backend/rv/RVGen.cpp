#include <backend/rv/RVGen.hpp>
void RVGen::init(Module *m)
{
    rvm = new RVModule;
}
void run(Module *m)
{
    init(m);
}