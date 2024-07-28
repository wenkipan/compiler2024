#pragma once

typedef void *yyscan_t;

#include "../../include/frontend/syntax_info.hpp"
#include "../../include/frontend/syntax_init.hpp"
#include "../../include/frontend/syntax_type_array.hpp"
#include "../../include/frontend/syntax_decl.hpp"
#include "../../include/frontend/syntax_decl_head.hpp"

// #include <grammar/SysY.yy.hpp>
// #include <grammar/SysY.tab.hpp>

#include "../../include/program/program.hpp"
#include "../../include/ast/ast.hpp"
#include "../../include/ir/Module.hpp"

p_program frontend_trans(const char *input, const char *output, Module *module);
