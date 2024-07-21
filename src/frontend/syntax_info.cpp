#include <frontend/syntax_info.hpp>
#include <string>
syntax_info::syntax_info(const char *input, const char *output)
    : p_table(new symbol_table()),
      _p_program(new program(input, output)),
      p_func(NULL),
      p_block(NULL),
      p_mem_set(NULL)
{
}
syntax_info::~syntax_info()
{
    delete (this->p_table);
    assert(!this->p_func);
}
void syntax_info::syntax_set_block(p_ast_block p_block)
{
    this->p_block = p_block;
}
void syntax_info::syntax_zone_push()
{
    this->p_table->symbol_table_zone_push();
}
void syntax_info::syntax_zone_pop()
{
    this->p_table->symbol_table_zone_pop();
}
p_symbol_var syntax_info::syntax_find_var(const char *name)
{
    return this->p_table->symbol_table_var_find(name);
}
p_symbol_func syntax_info::syntax_find_func(const char *name)
{
    return this->p_table->symbol_table_func_find(name);
}
p_symbol_str syntax_info::syntax_get_str(const char *string)
{
    p_symbol_str p_str = this->p_table->symbol_table_str_find(string);
    if (p_str)
        return p_str;

    p_str = this->p_table->symbol_table_str_add(string);
    this->_p_program->program_add_str(p_str);
    return p_str;
}

void syntax_info::syntax_func_add_variable(p_symbol_var p_var)
{
    this->p_table->symbol_table_var_add(p_var);
    this->p_func->symbol_func_add_variable(p_var);
}
void syntax_info::syntax_func_add_param(p_symbol_var p_var)
{
    this->p_table->symbol_table_var_add(p_var);
    this->p_func->symbol_func_add_param(p_var);
}

void syntax_info::syntax_program_add_variable(p_symbol_var p_var)
{
    this->p_table->symbol_table_var_add(p_var);
    this->_p_program->program_add_global(p_var);
    this->module->Gvar_add(p_var);
}
void syntax_info::syntax_program_add_function(p_symbol_func p_func)
{
    this->p_table->symbol_table_func_add(p_func);
    this->_p_program->program_add_function(p_func);
}

static inline bool syntax_init_by_assign_gen_no_zero(p_symbol_type p_type, p_syntax_init p_init, p_ast_exp p_addr, p_ast_block p_block)
{
    if (list_head_alone(&p_addr->p_type->array))
    {
        assert(p_addr->p_type->ref_level == 1);
        p_ast_exp p_rval = p_init->syntax_init_get_exp();
        assert(p_rval);
        if (p_rval->kind == ast_exp::ast_exp_num && p_rval->i32const == 0)
        {
            ast_exp_drop(p_rval);
            return false;
        }
        p_ast_stmt p_assign = ast_stmt_assign_gen(p_addr, p_rval);
        p_block->ast_block_add(p_assign);
        return true;
    }
    p_symbol_type_array p_arr = p_type->symbol_type_pop_array();
    p_ast_exp p_element_addr = new ast_exp(p_addr, new ast_exp((I32CONST_t)0), true);
    p_ast_exp p_first_addr = p_element_addr;

    bool have_assign = false;
    bool last_addr_hav_assign = false;

    p_list_head p_node, p_head = p_init->syntax_init_get_head();
    list_for_each(p_node, p_head)
    {
        p_syntax_init p_sub_init = syntax_init_get_entry(p_node);
        last_addr_hav_assign = syntax_init_by_assign_gen_no_zero(p_type, p_sub_init, p_element_addr, p_block);
        have_assign |= last_addr_hav_assign;
        if (p_node->p_next == p_head)
            continue;
        if (last_addr_hav_assign)
            p_element_addr = new ast_exp(p_element_addr->ast_exp_use_gen(), new ast_exp((I32CONST_t)1), false);
        else
            p_element_addr = new ast_exp(p_element_addr, new ast_exp((I32CONST_t)1), false);
    }
    if (!last_addr_hav_assign)
    {
        if (!have_assign)
        {
            // maybe problem
            p_first_addr->gep.p_addr = p_addr->ast_exp_use_gen();
        }
        ast_exp_drop(p_element_addr);
    }

    p_arr->symbol_type_push_array(p_type);
    return have_assign;
}
static inline size_t syntax_init_by_assign_gen(p_symbol_type p_type, p_syntax_init p_init, size_t index, p_ast_exp p_addr, p_ast_block p_block)
{
    if (list_head_alone(&p_addr->p_type->array))
    {
        assert(p_addr->p_type->ref_level == 1);
        p_ast_exp p_rval = p_init->syntax_init_find_exp(p_type, index++);
        if (!p_rval)
            p_rval = new ast_exp((I32CONST_t)0);
        p_ast_stmt p_assign = ast_stmt_assign_gen(p_addr, p_rval);
        p_block->ast_block_add(p_assign);
        return index;
    }
    p_ast_exp p_element_addr = new ast_exp(p_addr, new ast_exp((I32CONST_t)0), true);
    size_t length = (p_addr->p_type->size) / (p_element_addr->p_type->size);
    for (size_t i = 1; i < length; ++i)
    {
        index = syntax_init_by_assign_gen(p_type, p_init, index, p_element_addr, p_block);
        p_element_addr = new ast_exp(p_element_addr->ast_exp_use_gen(), new ast_exp((I32CONST_t)1), false);
    }
    return syntax_init_by_assign_gen(p_type, p_init, index, p_element_addr, p_block);
}
// maybe problem
p_syntax_decl_head syntax_info::syntax_declaration(p_syntax_decl_head p_head, p_syntax_decl p_decl)
{
    p_syntax_info p_info = this;
    // p_symbol_type p_type = p_decl->p_array->syntax_type_trans(p_head->type);
    p_symbol_type p_type = syntax_type_trans1(p_decl->p_array, p_head->type);
    p_syntax_init p_s_init = p_decl->p_init->syntax_init_regular(p_type);
    bool is_const = p_head->is_const;
    const char *name = p_decl->name;

    if (!p_info->p_func)
    {
        p_symbol_var p_var;
        if (p_s_init)
        {
            p_symbol_init p_init = new symbol_init(p_type->size, p_type->basic);
            for (size_t i = 0; i < p_init->size; ++i)
            {
                symbol_init_val init_val;
                p_ast_exp p_rval = p_s_init->syntax_init_find_exp(p_type, i);
                if (!p_rval)
                    p_rval = new ast_exp((I32CONST_t)0);
                else
                    p_rval = ast_exp_ptr_check_const(p_rval);
                if (p_init->basic == type_i32)
                {
                    init_val.i = p_rval->i32const;
                }
                else
                {
                    init_val.f = p_rval->f32const;
                }
                ast_exp_drop(p_rval);
                p_init->symbol_init_add(i, init_val);
            }
            p_var = new symbol_var(name, p_type, is_const, true, p_init);
        }
        else
        {
            assert(!is_const);
            p_var = new symbol_var(name, p_type, is_const, true, NULL);
        }
        p_info->syntax_program_add_variable(p_var);
    }
    else if (!p_info->p_block)
    {
        assert(!p_s_init);
        p_symbol_var p_var = new symbol_var(name, p_type, false, false, NULL);
        p_info->syntax_func_add_param(p_var);
    }
    else if (is_const)
    {
        p_symbol_init p_init = new symbol_init(p_type->size, p_head->type);
        for (size_t i = 0; i < p_type->size; ++i)
        {
            symbol_init_val init_val;
            p_ast_exp p_rval = p_s_init->syntax_init_find_exp(p_type, i);
            if (!p_rval)
                p_rval = new ast_exp((I32CONST_t)0);
            else
                p_rval = ast_exp_ptr_check_const(p_rval);
            if (p_init->basic == type_i32)
            {
                init_val.i = p_rval->i32const;
            }
            else
            {
                init_val.f = p_rval->f32const;
            }
            ast_exp_drop(p_rval);
            p_init->symbol_init_add(i, init_val);
        }
        p_symbol_var p_var = new symbol_var(name, p_type, true, false, p_init);
        p_info->syntax_func_add_variable(p_var);
    }
    else
    {
        p_symbol_var p_var = new symbol_var(name, p_type, false, false, NULL);
        p_info->syntax_func_add_variable(p_var);
        if (list_head_alone(&p_var->p_type->array))
        {
            p_ast_exp p_lval = new ast_exp(p_var);
            p_ast_exp p_rval = p_s_init->syntax_init_find_exp(p_type, 0);
            if (!p_rval)
                p_rval = new ast_exp((I32CONST_t)0);
            p_ast_stmt p_assign = ast_stmt_assign_gen(p_lval, p_rval);
            p_info->p_block->ast_block_add(p_assign);
        }
        else if (p_s_init)
        {
            if (p_var->p_type->size > 30)
            {
                p_ast_param_list p_pl = new ast_param_list();
                p_pl->ast_param_list_add(new ast_exp(p_var));
                p_pl->ast_param_list_add(new ast_exp((I32CONST_t)0));
                p_pl->ast_param_list_add(new ast_exp((I32CONST_t)p_var->p_type->size * (I32CONST_t)basic_type_get_size(p_var->p_type->basic)));
                p_ast_exp p_call_mem_set = new ast_exp(p_info->p_mem_set, p_pl);
                p_ast_stmt p_stmt = ast_stmt_exp_gen(p_call_mem_set);
                p_info->p_block->ast_block_add(p_stmt);
                p_ast_exp p_lval = new ast_exp(p_var);
                bool have_assign = syntax_init_by_assign_gen_no_zero(p_type, p_s_init, p_lval, p_info->p_block);
                if (!have_assign)
                    ast_exp_drop(p_lval);
            }
            else
            {
                p_ast_exp p_lval = new ast_exp(p_var);
                syntax_init_by_assign_gen(p_type, p_s_init, 0, p_lval, p_info->p_block);
            }
        }
    }
    delete (p_s_init);
    delete (p_decl);

    return p_head;
}

p_ast_stmt syntax_info::syntax_return(p_ast_exp p_exp)
{
    return ast_stmt_return_gen(this->p_func->ret_type, p_exp);
}

void syntax_info::syntax_func_head(basic_type type, char *name)
{
    p_syntax_info p_info = this;
    p_symbol_func p_func = new symbol_func(name, type, false);
    p_info->syntax_program_add_function(p_func);
    p_info->p_func = p_func;
    // maybe problem
    // free(name);
    delete[] name;

    p_info->syntax_zone_push();
}
void syntax_info::syntax_func_end(p_ast_block p_block)
{
    p_syntax_info p_info = this;
    p_info->syntax_zone_pop();
    p_symbol_func p_func = p_info->p_func;
    p_info->p_func = NULL;
    switch (p_func->ret_type)
    {
    case type_void:
        p_block->ast_block_add(ast_stmt_return_gen(type_void, NULL));
        break;
    case type_i32:
        p_block->ast_block_add(ast_stmt_return_gen(type_i32, new ast_exp((I32CONST_t)0)));
        break;
    case type_f32:
        p_block->ast_block_add(ast_stmt_return_gen(type_f32, new ast_exp((F32CONST_t)0.0)));
        break;
    case type_str:
        break;
    }
    // ast2ir_symbol_func_gen(p_block, p_func, p_info->_p_program);
    Function *_p_func = new Function(p_func);
    this->module->func_push_back(_p_func);
    _p_func->CallGen(p_block, p_func);
    // syntax_info_print_block(p_block);
    //  drop
    p_block->ast_block_drop();
}
static inline p_symbol_func syntax_rtlib_decl(p_syntax_info p_info, basic_type type, std::string name1, p_symbol_type p_param1, p_symbol_type p_param2, p_symbol_type p_param3, bool is_va)
{
    char *name = new char[name1.size() + 1];
    strcpy(name, name1.c_str());
    // leak
    p_symbol_func p_func = new symbol_func(name, type, is_va);
    delete[] name;
    p_info->syntax_program_add_function(p_func); // true
    Function *_p_func = new Function(p_func);
    _p_func->set_isExternal();
    p_info->module->func_push_back(_p_func);

    p_info->p_func = p_func;

    p_info->syntax_zone_push();
    if (p_param1)
    {
        p_symbol_var p_var = new symbol_var("arg1", p_param1, false, false, NULL);
        p_info->syntax_func_add_param(p_var);
        if (p_param2)
        {
            p_var = new symbol_var("arg2", p_param2, false, false, NULL);
            p_info->syntax_func_add_param(p_var);
            if (p_param3)
            {
                p_var = new symbol_var("arg3", p_param3, false, false, NULL);
                p_info->syntax_func_add_param(p_var);
            }
        }
    }
    p_info->syntax_zone_pop();
    p_info->p_func = NULL;
    return p_func;
}

void syntax_info::syntax_rtlib_func_init()
{
    p_syntax_info p_info = this;
    syntax_rtlib_decl(p_info, type_i32, "getint", NULL, NULL, NULL, false);
    syntax_rtlib_decl(p_info, type_i32, "getch", NULL, NULL, NULL, false);
    syntax_rtlib_decl(p_info, type_f32, "getfloat", NULL, NULL, NULL, false);

    p_symbol_type p_type = new symbol_type(type_i32);
    p_type->symbol_type_push_ptr();
    syntax_rtlib_decl(p_info, type_i32, "getarray", p_type, NULL, NULL, false);
    p_type = new symbol_type(type_f32);
    p_type->symbol_type_push_ptr();
    syntax_rtlib_decl(p_info, type_i32, "getfarray", p_type, NULL, NULL, false);

    syntax_rtlib_decl(p_info, type_void, "putint", new symbol_type(type_i32), NULL, NULL, false);
    syntax_rtlib_decl(p_info, type_void, "putch", new symbol_type(type_i32), NULL, NULL, false);
    syntax_rtlib_decl(p_info, type_void, "putfloat", new symbol_type(type_f32), NULL, NULL, false);

    p_type = new symbol_type(type_i32);
    p_type->symbol_type_push_ptr();
    syntax_rtlib_decl(p_info, type_void, "putarray", new symbol_type(type_i32), p_type, NULL, false);
    p_type = new symbol_type(type_f32);
    p_type->symbol_type_push_ptr();
    syntax_rtlib_decl(p_info, type_void, "putfarray", new symbol_type(type_i32), p_type, NULL, false);

    syntax_rtlib_decl(p_info, type_void, "putf", new symbol_type(type_str), NULL, NULL, true);

    syntax_rtlib_decl(p_info, type_void, "_sysy_starttime", new symbol_type(type_i32), NULL, NULL, false);
    syntax_rtlib_decl(p_info, type_void, "_sysy_stoptime", new symbol_type(type_i32), NULL, NULL, false);

    p_type = new symbol_type(type_void);
    p_type->symbol_type_push_ptr();
    p_info->p_mem_set = syntax_rtlib_decl(p_info, type_void, "memset", p_type, new symbol_type(type_i32), new symbol_type(type_i32), false);
}

// print
void ast_exp_print(p_ast_exp p_exp)
{
    switch (p_exp->kind)
    {
    case ast_exp::ast_exp_binary:
        printf("ast_exp_binary\n");
        break;
    case ast_exp::ast_exp_relational:
        printf("ast_exp_relational\n");
        break;
    case ast_exp::ast_exp_unary:
        printf("ast_exp_unary\n");
        break;
    case ast_exp::ast_exp_logic:
        printf("ast_exp_logic\n");
        break;
    case ast_exp::ast_exp_ulogic:
        printf("ast_exp_ulogic\n");
        break;
    case ast_exp::ast_exp_use:
        printf("ast_exp_use\n");
        break;
    case ast_exp::ast_exp_call:
        printf("ast_exp_call\n");
        break;
    case ast_exp::ast_exp_ptr:
        printf("ast_exp_ptr\n");
        break;
    case ast_exp::ast_exp_gep:
        printf("ast_exp_gep\n");
        break;
    case ast_exp::ast_exp_load:
        printf("ast_exp_load\n");
        break;
    case ast_exp::ast_exp_num:
        printf("ast_exp_num\n");
        break;
    default:
        assert(0);
    }
}
void ast_block_print(p_ast_block p_block)
{
    p_list_head p_node;
    list_for_each(p_node, &p_block->stmt)
    {
        p_ast_stmt p_stmt = list_entry(p_node, ast_stmt, node);
        ast_stmt_print(p_stmt);
    }
}
void ast_stmt_print(p_ast_stmt p_stmt)
{
    switch (p_stmt->type)
    {
    case ast_stmt::ast_stmt_assign:
        printf("ast_stmt_assign\n");
        ast_exp_print(p_stmt->array.p_lval);
        ast_exp_print(p_stmt->array.p_rval);
        break;
    case ast_stmt::ast_stmt_return:
        printf("ast_stmt_return\n");
        if (p_stmt->branch.p_exp != nullptr)
            ast_exp_print(p_stmt->branch.p_exp);
        break;
    case ast_stmt::ast_stmt_exp:
        printf("ast_stmt_exp\n");
        if (p_stmt->branch.p_exp)
            ast_exp_print(p_stmt->branch.p_exp);
        break;
    case ast_stmt::ast_stmt_block:
        printf("ast_stmt_block\n");
        ast_block_print(p_stmt->p_block);
        break;
    case ast_stmt::ast_stmt_if_else:
        printf("ast_stmt_if_else\n");
        ast_exp_print(p_stmt->branch.p_exp);
        ast_stmt_print(p_stmt->branch.p_stmt_1);
        ast_stmt_print(p_stmt->branch.p_stmt_2);
        break;
    case ast_stmt::ast_stmt_if:
        printf("ast_stmt_if\n");
        ast_exp_print(p_stmt->branch.p_exp);
        ast_stmt_print(p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt::ast_stmt_while:
        printf("ast_stmt_while\n");
        ast_exp_print(p_stmt->branch.p_exp);
        ast_stmt_print(p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt::ast_stmt_break:
        printf("ast_stmt_break\n");
        break;
    case ast_stmt::ast_stmt_continue:
        printf("ast_stmt_continue\n");
        break;
    default:
        assert(0);
    }
}
void syntax_info_print_block(p_ast_block p_block)
{
    printf("=== program block start ===\n");
    p_list_head p_node;
    list_for_each(p_node, &p_block->stmt)
    {
        p_ast_stmt p_stmt = list_entry(p_node, ast_stmt, node);
        ast_stmt_print(p_stmt);
    }
    printf("=== program block end ===\n");
}
