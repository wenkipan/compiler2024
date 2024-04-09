#pragma once

typedef void *yyscan_t;

#include <frontend/syntax_info.hpp>
#include <frontend/syntax_init.hpp>
#include <frontend/syntax_type_array.hpp>
#include <frontend/syntax_decl.hpp>
#include <frontend/syntax_decl_head.hpp>

// #include <grammar/SysY.yy.hpp>
// #include <grammar/SysY.tab.hpp>

#include <program/program.hpp>
#include <ast/ast.hpp>

p_program frontend_trans(const char *input, const char *output);
