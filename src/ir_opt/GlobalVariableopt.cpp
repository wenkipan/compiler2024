#include <ir_opt/GlobalVariableopt.hpp>
#include <vector>
void GlobalVariableopt::PassRun(Module *module)
{
    m = module;
    run();
}

void GlobalVariableopt::run()
{
    mark_live();
    replace_with_constant();
    demote_to_local();
}
void GlobalVariableopt::mark_live()
{
    for (auto gv : *m->get_globals())
        if (gv->get_user_list()->size() != 0)
            usefulGV.emplace(gv);
}
static Constant *get_GV_const(GlobalVariable *gv)
{
    assert(gv->get_type()->get_type() == TypeEnum::Ptr);
    TypeEnum a = ((Ptr *)gv->get_type())->get_btype()->get_type();
    assert(a == TypeEnum::I32 || a == TypeEnum::F32);
    Constant *c = nullptr;
    if (a == TypeEnum::I32)
    {
        if (gv->get_init())
        {
            assert(is_a<ConstantI32>(gv->get_init()));
            c = new ConstantI32(((ConstantI32 *)gv->get_init())->get_32_at(0));
        }
        else
            c = new ConstantI32(0);
    }
    else if (a == TypeEnum::F32)
    {
        if (gv->get_init())
        {
            assert(is_a<ConstantF32>(gv->get_init()));
            c = new ConstantF32(((ConstantF32 *)gv->get_init())->get_32_at(0));
        }
        else
            c = new ConstantF32(0.0);
    }
    else
        assert(0);
    return c;
}
void GlobalVariableopt::replace_with_constant()
{
    for (auto gv : usefulGV)
    {
        if (((Ptr *)gv->get_type())->get_btype()->isArray()) // array不做
            continue;

        int flag = 1;
        for (auto useredge : *gv->get_user_list())
        {
            if (!flag)
                break;
            assert(is_a<Instrution>(useredge->get_user()));
            Instrution *i = (Instrution *)useredge->get_user();
            if (!i->isLoad())
                flag = 0;
            else
            {
                for (auto useofi : *i->get_user_list())
                    if (((Instrution *)useofi->get_user())->isStore())
                        flag = 0;
            }
        }

        if (flag)
        {
            for (auto loadedge : *gv->get_user_list())
            {
                Load *load = (Load *)loadedge->get_user();
                Constant *c = get_GV_const(gv);
                load->get_parent()->get_func()->value_pushBack(c);
                load->replaceAllUses(c);
            }
        }
    }
}
void GlobalVariableopt::demote_to_local()
{
    std::queue<GlobalVariable *> deletelist;
    for (auto gv : usefulGV)
    {
        if (((Ptr *)gv->get_type())->get_btype()->isArray()) // array不做
            continue;
        Function *main = nullptr;
        int flag = 1;
        for (auto useredge : *gv->get_user_list())
        {
            assert(is_a<Instrution>(useredge->get_user()));
            Instrution *user = (Instrution *)useredge->get_user();
            if (user->get_parent()->get_func()->get_name() != "main")
            {
                flag = 0;
                break;
            }
            main = user->get_parent()->get_func();
        }
        if (flag)
        {
            assert(main);
            Instrution *newa = new Alloca(main->get_entryBB(), gv->get_type());
            newa->insertInstr(main->get_entryBB(), 0);
            Constant *c = get_GV_const(gv);
            main->value_pushBack(c);
            Instrution *newstore = new Store(newa, c, 0, main->get_entryBB());
            newstore->insertInstr(main->get_entryBB(), 1);
            for (auto useredge : *gv->get_user_list())
            {
                useredge->set_val(newa);
            }
            gv->get_user_list()->clear();
            deletelist.push(gv);
        }
    }

    while (!deletelist.empty())
    {
        GlobalVariable *gv = deletelist.front();
        deletelist.pop();
        for (auto it = m->get_globals()->begin(); it != m->get_globals()->end(); it++)
        {
            if (*it == gv)
            {
                m->get_globals()->erase(it);
                delete (gv);
                break;
            }
        }
    }

    // other left to mem2reg
}
// GEPS, useless ,consider gep offset is value
// void GlobalVariableopt::mark_as_constant()
// {
//     for (auto gv : usefulGV)
//     {
//         int had_store = 0;
//         for (auto useredge : *gv->get_user_list())
//         {
//             assert(is_a<Instrution>(useredge->get_user()));
//             Instrution *user = (Instrution *)useredge->get_user();
//             had_store = had_store & dfs_GEP(user);
//         }
//         if (had_store == 0)
//         {
//             // replaceusewithconstant();
//         }
//     }
// }
// static Constant *replaceusewithconstant(Instrution *i, GlobalVariable *GV, std::vector<int> poss)
// {
//     if (is_a<Load>(i))
//     {
//         assert(GV->get_type()->get_type() == TypeEnum::Ptr);
//         Constant *c = nullptr;
//         if (GV->get_init() == nullptr)
//         {
//             if (((Ptr *)GV->get_type())->get_btype()->get_basic_type() == TypeEnum::I32)
//                 c = new ConstantI32(0);
//             else if (((Ptr *)GV->get_type())->get_btype()->get_basic_type() == TypeEnum::F32)
//                 c = new ConstantF32((float)0);
//             else
//                 assert(0);
//         }
//         else
//         {
//             int pos = (poss, GV->get_type());
//             if (is_a<ConstantI32>(GV->get_init()))
//                 c = new ConstantI32(((ConstantI32 *)GV->get_init())->get_i32_at(pos));
//             else if (is_a<ConstantF32>(GV->get_init()))
//                 c = new ConstantF32(((ConstantF32 *)GV->get_init())->get_f32_at(pos));
//             else
//                 assert(0);
//         }

//         for (auto useredge : *i->get_user_list())
//         {
//             assert(is_a<Instrution>(useredge->get_user()));
//             useredge->set_user(c);
//         }
//     }

//     assert(is_a<GEP>(i));
//     for (auto useredge : *i->get_user_list())
//     {
//         assert(is_a<Instrution>(useredge->get_user()));
//         Instrution *user = (Instrution *)useredge->get_user();
//         GEP *i = (GEP *)i;
//         assert(is_a<ConstantI32>(i->get_offset()));
//         if (i->get_isele())
//         {
//             poss.push_back(0);
//         }
//         poss.push_back(((ConstantI32 *)i->get_offset())->get_i32_at(0));
//         replaceusewithconstant(user, GV, poss);
//     }
//     return;
// }
// bool GlobalVariableopt::dfs_GEP(Instrution *i)
// {
//     if (is_a<Load>(i))
//     {
//         for (auto useredge : *i->get_user_list())
//         {
//             assert(is_a<Instrution>(useredge->get_user()));
//             Instrution *user = (Instrution *)useredge->get_user();
//             if (is_a<Store>(user))
//                 return 1;
//         }
//         return 0;
//     }

//     assert(is_a<GEP>(i));
//     int ans = 0;
//     for (auto useredge : *i->get_user_list())
//     {
//         assert(is_a<Instrution>(useredge->get_user()));
//         Instrution *user = (Instrution *)useredge->get_user();
//         ans = ans & dfs_GEP(user);
//     }
//     return ans;
// }