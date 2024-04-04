#pragma once

#include <util.hpp>
#include <symbol.h>

typedef class symbol_str *p_symbol_str;
class symbol_str
{
public:
    char *string;
    size_t length;

    hlist_node h_node;
    list_head node;
    // p_symbol_str symbol_str_gen(const char *string);
    symbol_str(const char *string);
    // void symbol_str_drop(p_symbol_str p_str);
    ~symbol_str();
    // print
    void symbol_str_print();
};