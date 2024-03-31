#pragma once

#include <util.h>
#include <ir.h>
#include <symbol/func.h>
#include <symbol/str.h>
#include <symbol/type.h>
#include <symbol/var.h>
typedef class symbol_init_val *p_symbol_init_val;
typedef class symbol_init *p_symbol_init;
typedef class symbol_var *p_symbol_var;
typedef class symbol_func *p_symbol_func;
typedef class symbol_str *p_symbol_str;

typedef enum
{
    type_void,
    type_str,
    type_i32,
    type_f32,
} basic_type;
typedef class symbol_type *p_symbol_type;
typedef class symbol_type_array *p_symbol_type_array;
