#pragma once
#include <ir/ir.hpp>
#include <unistd.h>

void print_for_py(Function *func)
{
    int saved_stdout = dup(STDOUT_FILENO);
    freopen("./doc/edges.txt", "w", stdout);

    for (auto BB : *func->get_blocks())
    {
        for (auto succ : successors(BB))
            printf("%d %d\n", BB->get_ID(), succ->get_ID());
    }
    // freopen("/dev/tty", "w", stdout);
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}