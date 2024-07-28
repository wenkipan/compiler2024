#include <algorithm>

#include "../../include/ir_opt/DPE.hpp"

void DeadParamElimate::PassRun(Module *m)
{
    for (auto f : *m->get_funcs())
    {
        if (f->get_isExternal())
            continue;
        std::vector<int> paramspos;
        std::vector<Param *> deadparams;
        int i = 0;
        for (auto p : *f->get_params())
        {
            if (p->get_user_list()->size() == 0)
            {
                deadparams.push_back(p);
                paramspos.push_back(i);
            }
            i++;
        }
        for (auto p : deadparams)
        {
            auto it = std::find(f->get_params()->begin(), f->get_params()->end(), p);
            f->get_params()->erase(it);
            auto it2 = std::find(f->get_Values()->begin(), f->get_Values()->end(), p);
            f->get_Values()->erase(it2);
            delete p;
        }
        for (auto callee : *m->get_funcs())
        {
            if (callee->get_isExternal())
                continue;
            for (auto BB : *callee->get_blocks())
                for (auto i : *BB->get_instrs())
                    if (i->isCall() && i->get_operand_at(0) == f)
                    {
                        int dynamic = 0;
                        for (auto pos : paramspos)
                        {
                            printf("%d", pos);
                            printf("              ");
                            Edge *e = i->get_value_list()->at(1 + pos - dynamic);
                            e->get_val()->print_ID();
                            printf("\n");
                            e->drop();
                            dynamic++;
                        }
                    }
        }
    }
}