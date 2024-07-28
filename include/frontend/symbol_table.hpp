#pragma once

#include "../../include/symbol/symbol.hpp"
#include "../../include/symbol/var.hpp"
#include "../../include/symbol/func.hpp"
#include "../../include/symbol/str.hpp"

typedef class symbol_item *p_symbol_item;
typedef class symbol_name *p_symbol_name;
typedef class symbol_zone *p_symbol_zone;
typedef class symbol_table *p_symbol_table;

class symbol_item
{
public:
    p_symbol_name p_name;
    p_symbol_item p_prev;

    uint16_t level;
    p_symbol_item p_next;

    bool is_func;
    union
    {
        p_symbol_var p_var;
        p_symbol_func p_func;
    };
};

class symbol_name
{
public:
    char *name;
    p_symbol_item p_item;

    size_t hash_tag;
    hlist_node node;
};

class symbol_zone
{
public:
    p_symbol_item p_item;

    p_symbol_zone p_prev;
};

class symbol_table
{
public:
    hlist_hash hash;
    hlist_hash string_hash;

    uint16_t level;
    p_symbol_zone p_top_table;
    // p_symbol_table symbol_table_gen();
    symbol_table();
    // hash related
    // void symbol_table_drop(p_symbol_table p_table);
    ~symbol_table();
    void symbol_table_zone_pop();
    void symbol_table_zone_push();
    p_symbol_item symbol_table_var_add(p_symbol_var p_var);
    p_symbol_item symbol_table_func_add(p_symbol_func p_func);
    p_symbol_var symbol_table_var_find(const char *name);
    p_symbol_func symbol_table_func_find(const char *name);
    p_symbol_str symbol_table_str_find(const char *string);
    p_symbol_str symbol_table_str_add(const char *string);
};

#define hash_P (65537)
#define hash_MOD (109)
