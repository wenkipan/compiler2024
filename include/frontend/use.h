#pragma once

#include <ast/ast.h>
#include <program.h>
#include <syntax_info.h>

#include <grammar/SysY.yy.h>
#include <grammar/SysY.tab.h>

typedef void *yyscan_t;

p_program frontend_trans(const char *input, const char *output);

p_ast_exp syntax_val_offset(p_ast_exp p_val, p_ast_exp p_offset);

p_ast_stmt syntax_return(p_syntax_info p_info, p_ast_exp p_exp);

// log
#include <util/log.h>
void frontend_log(loglevel level, YYLTYPE *loc, yyscan_t scanner, const char *format, ...);

#define yyerror(loc, scan, fmt, ...) frontend_log(error, loc, scan, fmt, ##__VA_ARGS__)
#define yyinfo(loc, scan, fmt, ...) frontend_log(info, loc, scan, fmt, ##__VA_ARGS__)
#define yydebug(loc, scan, fmt, ...) frontend_log(debug, loc, scan, fmt, ##__VA_ARGS__)