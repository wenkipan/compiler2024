#include <ir_opt/DPE.hpp>
#include <algorithm>
void DeadParamElimate::run(Function *f)
{
    std::vector<Param *> deadparams;
    for (auto p : *f->get_params())
    {
        if (p->get_user_list()->size() == 0)
            deadparams.push_back(p);
    }

    for (auto p : deadparams)
    {
        auto it = std::find(f->get_params()->begin(), f->get_params()->end(), p);
        f->get_params()->erase(it);
        auto it2 = std::find(f->get_Values()->begin(), f->get_Values()->end(), p);
        f->get_Values()->erase(it2);
        delete p;
    }
}