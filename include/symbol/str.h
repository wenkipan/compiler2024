#pragma once

#include <util.h>
#include <ir.h>

class symbol_str
{
    char *string;
    size_t length;

    hlist_node h_node;
    list_head node;
    // gen
    p_symbol_str symbol_str_gen(const char *string);
    void symbol_str_drop(p_symbol_str p_str);
    // print
    void symbol_str_print(p_symbol_str p_str);
};