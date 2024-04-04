#pragma once

typedef void *yyscan_t;

#include <ast/ast.h>
#include <syntax_info.h>

p_ast_exp syntax_val_offset(p_ast_exp p_val, p_ast_exp p_offset);

p_ast_stmt syntax_return(p_syntax_info p_info, p_ast_exp p_exp);
