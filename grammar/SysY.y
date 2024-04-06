/*
 * SysY.y : Parser for SysY language
 */

%define parse.error verbose
%param { yyscan_t yyscanner }

%{
#include <frontend/lexer.hpp>
#include <frontend/log.hpp>

#include <frontend/syntax_init.hpp>
#include <frontend/syntax_decl_head.hpp>
#include <frontend/syntax_decl.hpp>

#include <ast/ast.hpp>

#define extra yyget_extra(yyscanner)

#define find_var(name) extra->syntax_find_var(name)
#define find_func(name) extra->syntax_find_func(name)
#define find_str(str) extra->syntax_get_str(str)
%}

%code requires{
#include <frontend/use.hpp>
}

%define api.pure full
%locations

%union {
       p_ast_block p_block;
       p_ast_stmt p_stmt;
       p_ast_exp p_exp;

       p_ast_param_list p_param_list;

       p_syntax_decl p_decl;
       p_syntax_decl_head p_decl_head;

       p_syntax_init p_init;

       basic_type type;

       char *ID;
       char *STRING;
       I32CONST_t I32CONST;
       F32CONST_t F32CONST;
}
%type <p_exp> Cond
%type <p_exp> LOrExp
%type <p_exp> LAndExp
%type <p_exp> EqExp
%type <p_exp> RelExp

%type <p_exp> Exp
%type <p_exp> ConstExp
%type <p_exp> AddExp
%type <p_exp> MulExp
%type <p_exp> UnaryExp
%type <p_exp> PrimaryExp

%type <p_exp> Call
%type <p_exp> Val
%type <p_exp> Str

%type <p_exp> StmtExp
%type <p_stmt> Stmt

%type <p_block> BlockItems
%type <p_block> Block

%type <p_param_list> FuncRParamList
%type <p_param_list> FuncRParams

%type <p_decl> ArraryParameter

%type <p_decl> Declarator
%type <p_decl> VarInitDeclarator
%type <p_decl_head> VarInitDeclaratorList
%type <p_decl> ConstInitDeclarator
%type <p_decl_head> ConstInitDeclaratorList

%type <p_init> VarInitializer
%type <p_init> VarInitializerList
%type <p_init> ConstInitializer
%type <p_init> ConstInitializerList

%type <type> Type

%locations

%token UNSIGNED SIGNED
%token LONG SHORT
%token INT FLOAT DOUBLE CHAR
%token VOID
%token CONST VOLATILE
%token DO WHILE FOR BREAK CONTINUE
%token IF ELSE
%token RETURN

%token AND OR LE GE EQ NEQ
%token SELFADD SELFSUB

%token <I32CONST> I32CONST
%token <F32CONST> F32CONST
%token <ID> ID
%destructor { free($$); } ID
%token <STRING> STRING
%destructor { free($$); } STRING

%nonassoc NO_ELSE
%nonassoc ELSE

%%
begin : PUSHZONE CompUnit POPZONE
      ;

CompUnit : CompUnit Declaration
         | CompUnit FuncDeclaration
         | CompUnit error
         | /* *empty */             { extra->syntax_rtlib_func_init(); }
         ;

Type : INT   { $$ = type_i32; }
     | FLOAT { $$ = type_f32; }
     ;

Declaration : ConstDeclaration
            | VarDeclaration
            ;

ConstDeclaration : ConstInitDeclaratorList ';' { delete($1); }
                 ;

VarDeclaration : VarInitDeclaratorList ';' { delete($1); }
               ;

ConstInitDeclaratorList : ConstInitDeclaratorList ',' ConstInitDeclarator { $$ = extra->syntax_declaration( $1, $3); }
                        | CONST Type ConstInitDeclarator                  { $$ = extra->syntax_declaration( new syntax_decl_head($2, true), $3); }
                        ;

VarInitDeclaratorList : VarInitDeclaratorList ',' VarInitDeclarator { $$ = extra->syntax_declaration( $1, $3); }
                      | Type VarInitDeclarator                      { $$ = extra->syntax_declaration( new syntax_decl_head($1, false), $2); }
                      ;

ConstInitDeclarator : Declarator '=' ConstInitializer { $$ = $1->syntax_decl_init( $3); }
                    ;

VarInitDeclarator : Declarator '=' VarInitializer { $$ = $1->syntax_decl_init( $3); }
                  | Declarator                    { $$ = $1->syntax_decl_init( NULL); }
                  ;

Declarator : Declarator '[' ConstExp ']' { $$ = $1->syntax_decl_arr( $3); }
           | ID                          { $$ = new syntax_decl($1); }
           | Declarator error
           ;

ConstInitializer : '{' ConstInitializerList '}'     { $$ = $2; }
                 | '{' '}'                          { $$ = new syntax_init(); }
                 | ConstExp                         { $$ = new syntax_init($1); }
                 ;

ConstInitializerList : ConstInitializerList ',' ConstInitializer { $$ = $1->syntax_init_list_add($3); }
                     | ConstInitializer                          { $$ = new syntax_init();$$->syntax_init_list_add($1); }
                     ;

VarInitializer : '{' VarInitializerList '}'     { $$ = $2; }
               | '{' '}'                        { $$ = new syntax_init(); }
               | Exp                      { $$ = new syntax_init($1); }
               ;

VarInitializerList : VarInitializerList ',' VarInitializer { $$ = $1->syntax_init_list_add( $3); }
                   | VarInitializer                        { $$ = new syntax_init();$$->syntax_init_list_add($1); }
                   ;

FuncHead : Type ID { extra->syntax_func_head( $1, $2); }
         | VOID ID { extra->syntax_func_head( type_void, $2); }
         ;

FuncDeclaration : FuncHead '(' Parameters ')' Block { extra->syntax_func_end( $5); }
                ;

Parameters : ParameterList
           | /* *empty */
           ;

ParameterList : ParameterList ',' ParameterDeclaration
              | ParameterDeclaration
              ;

ParameterDeclaration : Type ArraryParameter { p_syntax_decl_head p_head = new syntax_decl_head($1, false); extra->syntax_declaration( p_head, $2); delete(p_head); }
                     | Type ID              { p_syntax_decl_head p_head = new syntax_decl_head($1, false); extra->syntax_declaration( p_head, new syntax_decl($2)); delete(p_head); }
                     ;

ArraryParameter : ID '[' ']'                  { ;$$ = new syntax_decl($1); $$->syntax_decl_arr( NULL); }
                | ArraryParameter '[' Exp ']' { $$ = $1->syntax_decl_arr( $3); }
                ;

Cond : LOrExp
     ;

LOrExp : LOrExp OR LAndExp { $$ = new ast_exp((ast_exp_logic_op)ast_exp_op_bool_or, $1, $3); }
       | LAndExp
       ;

LAndExp : LAndExp AND EqExp { $$ = new ast_exp((ast_exp_logic_op)ast_exp_op_bool_and, $1, $3); }
        | EqExp             { $$ = $1->ast_exp_to_cond(); }
        ;

EqExp : EqExp EQ RelExp  { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_eq, $1, $3); }
      | EqExp NEQ RelExp { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_neq, $1, $3); }
      | RelExp
      ;

RelExp : RelExp '<' AddExp { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_l, $1, $3); }
       | RelExp '>' AddExp { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_g, $1, $3); }
       | RelExp LE AddExp  { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_leq, $1, $3); }
       | RelExp GE AddExp  { $$ = new ast_exp((ast_exp_relational_op)ast_exp_op_geq, $1, $3); }
       | AddExp
       ;

ConstExp : Exp { $$ = ast_exp_ptr_check_const($1); }
         ;

Exp : AddExp
    ;

AddExp : AddExp '+' MulExp { $$ = ast_exp_binary_gen(ast_exp_op_add, $1, $3); }
       | AddExp '-' MulExp { $$ = ast_exp_binary_gen(ast_exp_op_sub, $1, $3); }
       | MulExp
       ;

MulExp : MulExp '*' UnaryExp { $$ = ast_exp_binary_gen(ast_exp_op_mul, $1, $3); }
       | MulExp '/' UnaryExp { $$ = ast_exp_binary_gen(ast_exp_op_div, $1, $3); }
       | MulExp '%' UnaryExp { $$ = ast_exp_binary_gen(ast_exp_op_mod, $1, $3); }
       | UnaryExp
       ;

UnaryExp : '-' UnaryExp     { $$ = ast_exp_unary_gen(ast_exp_op_minus, $2); }
         | '+' UnaryExp     { $$ = $2; }
         | '!' UnaryExp     { $$ = new ast_exp((ast_exp_ulogic_op)ast_exp_op_bool_not, $2); }
         | PrimaryExp
         ;

PrimaryExp : '(' Exp ')' { $$ = $2; }
           | I32CONST    { $$ = new ast_exp((I32CONST_t)$1); }
           | F32CONST    { $$ = new ast_exp((F32CONST_t)$1); }
           | Val         { $$ = $1; }
           | Call        { $$ = $1; }
           | Str         { $$ = $1; }
           ;

Call : ID '(' FuncRParams ')' { $$ = new ast_exp(find_func($1), $3); free($1); }
     ;

Val : ID                 { $$ = new ast_exp(find_var($1)); free($1); }
    | Val '[' Exp ']'    { $$ = syntax_val_offset($1, $3); }
    ;

Str : STRING { $$ = new ast_exp(find_str($1)); free($1); }
    ;

FuncRParams : FuncRParamList { $$ = $1; }
            | /* *empty */   { $$ = new ast_param_list(); }
            ;

FuncRParamList : FuncRParamList ',' Exp { $$ = $1->ast_param_list_add( $3); }
               | Exp                    { $$ = new ast_param_list();$$->ast_param_list_add( $1); }
               ;

Block : '{' BlockItems '}' { $$ = $2; extra->syntax_set_block( NULL); }
      ;

BlockItems : BlockItems Declaration
           | BlockItems Stmt           { $$ = $1->ast_block_add( $2); extra->syntax_set_block( $$); }
           | /* *empty */              { $$ = new ast_block(); extra->syntax_set_block( $$); }
           ;

StmtExp : /* *empty */ { $$ = NULL; }
        | Exp
        ;

Stmt : PUSHZONE Block POPZONE             { $$ = ast_stmt_block_gen($2); }
     | Val '=' Exp ';'                    { $$ = ast_stmt_assign_gen($1, $3); }
     | StmtExp ';'                        { $$ = ast_stmt_exp_gen($1); }
     | RETURN StmtExp ';'                 { $$ = extra->syntax_return($2); } 
     | BREAK ';'                          { $$ = ast_stmt_break_gen(); }
     | CONTINUE ';'                       { $$ = ast_stmt_continue_gen(); }
     | IF '(' Cond ')' Stmt ELSE Stmt     { $$ = ast_stmt_if_else_gen($3, $5, $7); }
     | IF '(' Cond ')' Stmt %prec NO_ELSE { $$ = ast_stmt_if_gen($3, $5); }
     | WHILE '(' Cond ')' Stmt            { $$ = ast_stmt_while_gen($3, $5); }
     | error                              { $$ = ast_stmt_exp_gen(NULL); }
     ;

PUSHZONE : /* *empty */ { extra->syntax_zone_push(); }
         ;

POPZONE : /* *empty */ { extra->syntax_zone_pop(); }
        ;
%%
