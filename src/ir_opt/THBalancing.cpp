#include "ir/BasicBlock.hpp"
#include "ir/Instrution.hpp"
#include "ir/Value.hpp"
#include <ir_opt/THBalancing.hpp>
#include <vector>

static inline int is_root(Instrution *i)
{
    if (i->get_user_list()->size() > 1)
        return 1;
    return i->get_Instrtype() != ((Instrution *)i->get_user_list()->at(0)->get_user())->get_Instrtype();
}
int THBalancing::get_rank(Value *i)
{
    assert(rank.find(i) != rank.end());
    return rank.find(i)->second;
}
void THBalancing::set_rank(Value *i, int r)
{
    rank.emplace(i, r);
}
static inline int is_leaf(Instrution *root, Value *var, BasicBlock *bb)
{
    if (is_a<Param>(var))
        return 1;
    assert(is_a<Instrution>(var));
    if (is_a<PHINode>(var))
        return 1;

    if (((Instrution *)var)->get_BB() != bb)
        return 1;
    //  def of UEV_AR(bb) above

    if (root->get_Instrtype() != ((Instrution *)var)->get_Instrtype())
        return 1;

    return 0;
}
void THBalancing::run(Function *func)
{
    f = func;
    for (auto BB : *f->get_blocks())
    {
        curbb = BB;
        for (auto instr : *BB->get_instrs())
        {
            rank.emplace(instr, -1);
            if (instr->isIBinary() && instr->is_commutative() && is_root(instr))
            {
                rootqueue.emplace(instr);
                rootset.emplace(instr);
            }
        }
        while (!rootqueue.empty())
        {
            Instrution *root = rootqueue.top();
            rootqueue.pop();
            if (if_debug)
            {
                printf("root:");
                root->print();
            }
            balance(root);
        }
    }
}
Instrution *THBalancing::balance(Instrution *root)
{
    if (get_rank(root) >= 0)
        return root;

    Value *l = root->get_operand_at(0);
    Value *r = root->get_operand_at(1);
    std::priority_queue<Value *, std::vector<Value *>, decltype(rankcmp)> q(rankcmp);
    Flatten(root, l, q);
    Flatten(root, r, q);
    return rebuild(root, q);
}
void THBalancing::Flatten(Instrution *root, Value *var, std::priority_queue<Value *, std::vector<Value *>, decltype(rankcmp)> &q)
{
    if (is_a<Constant>(var))
    {
        if (if_debug)
        {
            printf("push:");
            var->print();
            printf("\n");
        }
        rank.emplace(var, 0);
        q.push(var);
    }
    else if (rootset.find((Instrution *)var) != rootset.end()) // var is a root
    {
        assert(is_a<Instrution>(var));
        Instrution *newroot = balance((Instrution *)var);
        if (if_debug)
        {
            printf("push:");
            newroot->print();
        }
        q.push(newroot);
    }
    else if (is_leaf(root, var, curbb)) // leaf for the root
    {

        assert(is_a<Instrution>(var) || is_a<Param>(var));
        if (if_debug)
        {
            printf("push:");
            var->print();
            printf("\n");
        }
        rank.emplace(var, 1);
        q.push(var);
    }
    else // recur
    {
        assert(is_a<Instrution>(var));
        Flatten(root, ((Instrution *)var)->get_operand_at(0), q);
        Flatten(root, ((Instrution *)var)->get_operand_at(1), q);
    }
}
Instrution *THBalancing::rebuild(Instrution *root, std::priority_queue<Value *, std::vector<Value *>, decltype(rankcmp)> &q)
{
    printf("rebuildroot:");
    root->print();
    while (!q.empty())
    {
        Value *NL = q.top();
        q.pop();
        Value *NR = q.top();
        q.pop();
        if (if_debug)
        {
            NL->print_ID();
            printf("NLRANK:%d\n", get_rank(NL));
            NR->print_ID();
            printf("NRRANK:%d\n", get_rank(NR));
        }

        Instrution *NT = new Binary(root->get_Instrtype(), NL, NR, curbb);
        NT->insertInstr(root->get_parent(), root->get_pos_of_bb());
        if (q.empty())
        {
            root->replaceAllUses(NT);
            rootset.emplace(NT); // careful,reuse rootset for recursive
            rank.emplace(NT, 1);
            return NT;
        }

        if (is_a<Constant>(NL) && is_a<Constant>(NR))
            rank.emplace(NT, 0);
        else
            rank.emplace(NT, get_rank(NL) + get_rank(NR));

        if (!q.empty())
        {
            if (if_debug)
            {
                printf("push:");
                NT->print();
            }
            q.push(NT);
        }
    }

    assert(0);
}
