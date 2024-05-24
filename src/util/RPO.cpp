#include <util/RPO.hpp>
#include <algorithm>

static void inline visit(BasicBlock *bb, std::set<BasicBlock *> &visited, std::vector<BasicBlock *> &order)
{
    if (visited.find(bb) != visited.end())
        return;

    visited.emplace(bb);
    for (auto succ : successors(bb))
        visit(succ, visited, order);
    order.push_back(bb);
}
std::vector<BasicBlock *> RPO(Function *f)
{
    std::vector<BasicBlock *> order;
    std::set<BasicBlock *> visited;
    visit(f->get_entryBB(), visited, order);
    std::reverse(order.begin(), order.end());
    return order;
}