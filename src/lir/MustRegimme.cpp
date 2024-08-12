#include <algorithm>

#include "../../include/lir/MustRegimme.hpp"
#include "../../include/lir/ArmStand.hpp"

// static inline bool is_move_case(Instrution *i)
// {
//     if (i->isBinary() && (i->get_Instrtype() == InstrutionEnum::IMUL || i->get_Instrtype() == InstrutionEnum::IDIV))
//         return true;
//     return false;
// }
void MustRegimme::run(Module *m)
{
    // imul op must be reg,float must be reg
    for (auto f : *m->get_funcs())
    {
        if (f->get_isExternal())
            continue;
        for (auto BB : *f->get_blocks())
        {
            auto workvec = *BB->get_instrs();
            for (auto i : workvec)
            {
                if (i->isBinary() && (i->get_Instrtype() == InstrutionEnum::IMUL || i->get_Instrtype() == InstrutionEnum::IDIV))
                {
                    assert(i->get_value_list()->size() == 2);
                    for (auto valedge : *i->get_value_list())
                    {
                        Value *src = valedge->get_val();
                        if (is_a<ConstantI32>(src))
                        {
                            Instrution *newl = new Assign(InstrutionEnum::Assign, src, BB);
                            auto v = i->get_parent()->get_instrutions();
                            int pos = 0;
                            for (auto it = v->begin(); it != v->end(); it++, pos++)
                                if ((*it) == i)
                                    break;
                            newl->insertInstr(BB, pos);
                            // delete const src's edge to i  const->mul
                            auto it = find(src->get_user_list()->begin(), src->get_user_list()->end(), valedge);
                            assert(it != src->get_user_list()->end());
                            src->get_user_list()->erase(it);
                            // change edge
                            valedge->set_val(newl);
                        }
                    }
                }
            }
        }
    }

    // for (auto f : *m->get_funcs())
    // {
    //     if (f->get_isExternal())
    //         continue;
    //     for (auto BB : *f->get_blocks())
    //     {
    //         auto workvec = *BB->get_instrs();
    //         for (auto i : workvec)
    //         {
    //             for (auto valedge : *i->get_value_list())
    //             {
    //                 Value *src = valedge->get_val();
    //                 if (is_a<ConstantF32>(src))
    //                 {
    //                     Instrution *newl = new Assign(InstrutionEnum::Assign, src, BB);
    //                     auto v = i->get_parent()->get_instrutions();
    //                     int pos = 0;
    //                     for (auto it = v->begin(); it != v->end(); it++, pos++)
    //                         if ((*it) == i)
    //                             break;
    //                     newl->insertInstr(BB, pos);
    //                     // delete const src's edge to i  const->mul
    //                     auto it = find(src->get_user_list()->begin(), src->get_user_list()->end(), valedge);
    //                     assert(it != src->get_user_list()->end());
    //                     src->get_user_list()->erase(it);
    //                     // change edge
    //                     valedge->set_val(newl);
    //                 }
    //             }
    //         }
    //     }
    // }
}
