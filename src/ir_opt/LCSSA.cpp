#include <ir_opt/LCSSA.hpp>
void LCSSA::PassRun(Module *p_module)
{
    LA.PassRun(p_module);
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        AnalysisForFunc(p_func);
    }
}

void LCSSA::replaceUsesOutofLoop(Instrution *Ins, Value *RepVal, Loop *loop)
{
    for (auto edge = Ins->get_user_list()->begin(); edge != Ins->get_user_list()->end();)
    {
        BasicBlock *BB = dynamic_cast<Instrution *>((*edge)->get_user())->get_parent();
        if (loop->get_BBs()->find(BB) == loop->get_BBs()->end())
        {
            (*edge)->set_val(RepVal);
            edge = Ins->get_user_list()->erase(edge);
        }
        else
            edge++;
    }
}

void LCSSA::AnalysisForLoop(Loop *p_loop)
{
    std::set<BasicBlock *> *BBs = p_loop->get_BBs();
    for (BasicBlock *BB : *BBs)
    {
        for (Instrution *Ins : *(BB->get_instrs()))
        {
            if (Ins->get_type()->get_type() == TypeEnum::Void)
                continue;
            for (Edge *edge : *Ins->get_user_list())
            {
                Instrution *user = dynamic_cast<Instrution *>(edge->get_user());
                if (!dynamic_cast<PHINode *>(user) && BBs->find(user->get_parent()) == BBs->end())
                {
                    PHINode *phi = new PHINode(user->get_parent(), Ins->get_type()->get_type(), true);
                    user->get_parent()->Insert_Phi(phi);
                    replaceUsesOutofLoop(Ins, phi, p_loop);
                    phi->addIncoming(Ins, BB);
                }
            }
        }
    }
}

void LCSSA::AnalysisForFunc(Function *p_func)
{
    Loop *root = LA.get_LoopInfo()->find(p_func)->second;
    for (Loop *lp : *(root->get_lpsons()))
    {
        AnalysisForLoop(lp);
    }
}
