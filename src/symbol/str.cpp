#include <stdio.h>

#include "../../include/symbol/str.hpp"

symbol_str::symbol_str(const char *string)
    : string(new char[strlen(string) + 1]),
      length(strlen(string)),
      h_node(hlist_init_node),
      node(list_head_init((&this->node)))
{
    strcpy(this->string, string);
}
symbol_str::~symbol_str()
{
    list_del(&this->node);
    delete[] this->string;
}
void symbol_str::symbol_str_print()
{
    printf("\"%s\"", this->string);
}