#include <ast/ast.hpp>
// ast_block
ast_block::ast_block()
    : length(0),
      stmt(list_head_init(&this->stmt))
{
}
p_ast_block ast_block::ast_block_add(p_ast_stmt p_stmt)
{
    p_ast_block p_block = this;
    if (p_stmt->type != ast_stmt::ast_stmt_block)
    {
        if (p_stmt->type == ast_stmt::ast_stmt_exp && p_stmt->branch.p_exp == NULL)
        {
            free(p_stmt);
        }
        else
        {
            ++p_block->length;
            list_add_prev(&p_stmt->node, &p_block->stmt);
        }
    }
    else
    {
        p_block->length += p_stmt->p_block->length;
        if (!list_blk_add_prev(&p_stmt->p_block->stmt, &p_block->stmt))
            list_replace(&p_block->stmt, &p_stmt->p_block->stmt);
        free(p_stmt->p_block);
        free(p_stmt);
    }
    return p_block;
}
ast_block::~ast_block()
{
    // assert(this);
    // while (!list_head_alone(&this->stmt))
    // {
    //     p_ast_stmt p_stmt = list_entry(this->stmt.p_next, ast_stmt, node);
    //     delete (p_stmt);
    // }
    // free(p_block);
}
void ast_block::ast_block_drop()
{
    while (!list_head_alone(&this->stmt))
    {
        p_ast_stmt p_stmt = list_entry(this->stmt.p_next, ast_stmt, node);
        delete (p_stmt);
    }
    delete (this);
}

// ast_param_list
ast_param_list::ast_param_list()
    : param(list_head_init(&this->param))
{
}
p_ast_param_list ast_param_list::ast_param_list_add(p_ast_exp p_exp)
{

    // printf("%s\n", p_exp->p_var->name);
    p_exp = p_exp->ast_exp_ptr_to_val();
    // printf("tttt\n");
    p_ast_param p_param = new ast_param(p_exp);

    list_add_prev(&p_param->node, &this->param);
    return this;
}
ast_param_list::~ast_param_list()
{
    assert(this);
    while (!list_head_alone(&this->param))
    {
        p_ast_param p_param = list_entry(this->param.p_next, ast_param, node);
        // ast_param_drop(p_param);
        delete (p_param);
    }
    // free(p_param_list);
}
ast_param::ast_param(p_ast_exp p_exp)
    : is_stck_ptr(p_exp->p_type->ref_level > 0 ? p_exp->ast_exp_ptr_is_stack() : false),
      p_exp(p_exp),
      node(list_head_init(&this->node))
{
}
ast_param::~ast_param()
{
    assert(this);
    list_del(&this->node);
    delete (this->p_exp);
    // free(p_param);
}
// ast_stmt
ast_stmt::ast_stmt(basic_type ret_type, p_ast_exp p_exp)
    : type(ast_stmt_return),
      node(list_head_init(&this->node))
{
    if (p_exp)
    {
        p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
        p_exp = p_exp->ast_exp_cov_gen(ret_type);
    }
    else
    {
        assert(ret_type == type_void);
    }
    this->branch.p_exp = p_exp;
}
p_ast_stmt ast_stmt_return_gen(basic_type ret_type, p_ast_exp p_exp)
{
    return new ast_stmt(ret_type, p_exp);
}
ast_stmt::ast_stmt(p_ast_exp p_exp)
    : type(ast_stmt_exp),
      branch{p_exp, NULL, NULL},
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_exp_gen(p_ast_exp p_exp)
{
    if (p_exp)
        p_exp = p_exp->ast_exp_ptr_to_val();
    return new ast_stmt(p_exp);
}
ast_stmt::ast_stmt()
    : type(),
      null(NULL),
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_break_gen(void)
{
    p_ast_stmt re = new ast_stmt();
    re->type = ast_stmt::ast_stmt_break;
    return re;
}
p_ast_stmt ast_stmt_continue_gen(void)
{
    p_ast_stmt re = new ast_stmt();
    re->type = ast_stmt::ast_stmt_continue;
    return re;
}
ast_stmt::ast_stmt(p_ast_exp p_exp, p_ast_stmt p_stmt_1)
    : type(ast_stmt_if),
      branch{p_exp, p_stmt_1, NULL},
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_if_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1)
{
    assert(p_exp && p_stmt_1);
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
    return new ast_stmt(p_exp, p_stmt_1);
}
ast_stmt::ast_stmt(p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2)
    : type(ast_stmt_if_else),
      branch{p_exp, p_stmt_1, p_stmt_2},
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_if_else_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2)
{
    assert(p_exp && p_stmt_1 && p_stmt_2);
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
    return new ast_stmt(p_exp, p_stmt_1, p_stmt_2);
}
p_ast_stmt ast_stmt_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1)
{
    assert(p_exp && p_stmt_1);
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
    p_ast_stmt re = new ast_stmt(p_exp, p_stmt_1);
    re->type = ast_stmt::ast_stmt_while;
    return re;
}
ast_stmt::ast_stmt(p_ast_block p_block)
    : type(ast_stmt_block),
      p_block(p_block),
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_block_gen(p_ast_block p_block)
{
    assert(p_block);
    if (p_block->length == 0)
    {
        delete (p_block);
        return ast_stmt_exp_gen(NULL);
    }
    if (p_block->length == 1)
    {
        p_ast_stmt p_stmt = list_entry(p_block->stmt.p_next, ast_stmt, node);
        list_del(&p_stmt->node);
        delete (p_block);
        return p_stmt;
    }
    return new ast_stmt(p_block);
}
ast_stmt::ast_stmt(p_ast_exp lval, p_ast_exp rval)
    : type(ast_stmt_assign),
      array{lval->ast_exp_ptr_is_stack(), lval, rval},
      node(list_head_init(&this->node))
{
}
p_ast_stmt ast_stmt_assign_gen(p_ast_exp lval, p_ast_exp rval)
{
    assert(lval && rval);
    lval->ast_exp_ptr_check_lval();
    rval = rval->ast_exp_ptr_to_val_check_basic();
    rval = rval->ast_exp_cov_gen(lval->p_type->basic);
    return new ast_stmt(lval, rval);
}
ast_stmt::~ast_stmt()
{
    p_ast_stmt p_stmt = this;
    assert(p_stmt);
    list_del(&p_stmt->node);
    switch (p_stmt->type)
    {
    case ast_stmt_assign:
        delete (p_stmt->array.p_lval);
        delete (p_stmt->array.p_rval);
        break;
    case ast_stmt_block:
        p_stmt->p_block->ast_block_drop();
        break;
    case ast_stmt_exp:
        if (p_stmt->branch.p_exp)
            delete (p_stmt->branch.p_exp);
        break;
    case ast_stmt_return:
        if (p_stmt->branch.p_exp)
            delete (p_stmt->branch.p_exp);
        break;
    case ast_stmt_if_else:
        delete (p_stmt->branch.p_exp);
        delete (p_stmt->branch.p_stmt_1);
        delete (p_stmt->branch.p_stmt_2);
        break;
    case ast_stmt_while:
        delete (p_stmt->branch.p_exp);
        delete (p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt_if:
        delete (p_stmt->branch.p_exp);
        delete (p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt_break:
    case ast_stmt_continue:
        break;
    }
    // free(p_stmt);
}
// ast_exp
static inline p_ast_exp exp_ptr_to_val(p_ast_exp p_exp)
{
    // printf("exp_ptr_to_val::%lu\n", p_exp->p_type->ref_level);
    if (p_exp->p_type->ref_level == 0)
    {
        assert(list_head_alone(&p_exp->p_type->array));
        return p_exp;
    }
    if (p_exp->p_type->ref_level > 1)
    {
        return p_exp->ast_exp_load_gen();
    }
    // assert(p_exp->p_type->ref_level == 1);
    if (list_head_alone(&p_exp->p_type->array))
    {
        return p_exp->ast_exp_load_gen();
    }
    // printf("new exp\n");
    return new ast_exp(p_exp, new ast_exp((I32CONST_t)0), true);
}
static inline void exp_check_basic(p_ast_exp p_exp)
{
    assert(list_head_alone(&p_exp->p_type->array));
    // printf(":refl:%lu\n", p_exp->p_type->ref_level);
    // printf(":basic:%d\n", p_exp->p_type->basic);
    assert(p_exp->p_type->ref_level == 0);
    assert(p_exp->p_type->basic != type_void);
}

static inline p_ast_exp exp_ptr_to_val_check_basic(p_ast_exp p_exp)
{
    // printf("exp_ptr_to_val_check_basic begin\n");
    p_exp = exp_ptr_to_val(p_exp);

    // printf("exp_ptr_to_val_check_basic %lu\n", p_exp->p_type->ref_level);
    exp_check_basic(p_exp);
    return p_exp;
}

static inline p_ast_exp exp_val_const(p_ast_exp p_exp)
{
    if (p_exp->p_type->ref_level != 0)
        return p_exp;
    assert(list_head_alone(&p_exp->p_type->array));

    p_ast_exp p_back = p_exp;
    size_t offset = 0;
    size_t length = 1;
    p_exp = p_exp->load.p_ptr;
    while (p_exp->kind == ast_exp::ast_exp_gep)
    {
        if (p_exp->gep.p_offset->kind != ast_exp::ast_exp_num)
            break;
        if (p_exp->gep.p_offset->p_type->basic != type_i32)
            break;
        length *= (p_exp->p_type->size);
        offset += length * p_exp->gep.p_offset->i32const;
        p_exp = p_exp->gep.p_addr;
    }
    if (p_exp->kind == ast_exp::ast_exp_ptr && p_exp->p_var->is_const)
    {
        assert(offset < (p_exp->p_var->p_type->size));
        p_ast_exp p_val;
        if (p_exp->p_type->basic == type_i32)
        {
            p_val = new ast_exp((I32CONST_t)p_exp->p_var->p_init->memory[offset].i);
        }
        else
        {
            assert(p_exp->p_var->p_init);
            p_val = new ast_exp((F32CONST_t)p_exp->p_var->p_init->memory[offset].f);
        }
        delete (p_back);

        return p_val;
    }
    return p_back;
}

ast_exp::ast_exp(ast_exp_relational_op op, p_ast_exp p_rsrc_1, p_ast_exp p_rsrc_2)
{
    assert(p_rsrc_1 && p_rsrc_2);
    p_rsrc_1 = exp_ptr_to_val_check_basic(p_rsrc_1);
    p_rsrc_2 = exp_ptr_to_val_check_basic(p_rsrc_2);
    if (p_rsrc_1->p_type->basic == type_f32)
    {
        p_rsrc_2 = p_rsrc_2->ast_exp_cov_gen(type_f32);
    }
    else if (p_rsrc_2->p_type->basic == type_f32)
    {
        p_rsrc_1 = p_rsrc_1->ast_exp_cov_gen(type_f32);
    }
    this->r = {p_rsrc_1, p_rsrc_2, op};
    this->kind = ast_exp_relational;
    this->p_type = new symbol_type(type_i32);
}
ast_exp::ast_exp(ast_exp_logic_op l_op, p_ast_exp p_bool_1, p_ast_exp p_bool_2)
{
    assert(p_bool_1 && p_bool_2);
    p_bool_1 = exp_ptr_to_val_check_basic(p_bool_1);
    p_bool_2 = exp_ptr_to_val_check_basic(p_bool_2);

    p_bool_1 = p_bool_1->ast_exp_to_cond();
    p_bool_2 = p_bool_2->ast_exp_to_cond();

    this->l = {p_bool_1, p_bool_2, l_op};
    this->kind = ast_exp_logic;
    this->p_type = new symbol_type(type_i32);
}
ast_exp::ast_exp(ast_exp_ulogic_op ul_op, p_ast_exp p_bool)
{
    assert(p_bool);
    p_bool = exp_ptr_to_val_check_basic(p_bool);

    p_bool = p_bool->ast_exp_to_cond();

    this->ul = {p_bool, ul_op};
    this->kind = ast_exp_ulogic;
    this->p_type = new symbol_type(type_i32);
}
static inline bool param_arr_check(p_symbol_type p_type_f, p_symbol_type p_type_r)
{
    if (p_type_f->ref_level != p_type_r->ref_level)
        return false;
    if (p_type_f->ref_level == 1 && p_type_f->basic == type_void)
    {
        return p_type_r->ref_level == 1;
    }
    if (p_type_f->basic != p_type_r->basic)
        return false;
    p_list_head p_node_f, p_node_r = p_type_r->array.p_prev;
    list_for_each_tail(p_node_f, &p_type_f->array)
    {
        if (p_node_r == &p_type_r->array)
            return false;
        p_symbol_type_array p_array_f, p_array_r;
        p_array_f = list_entry(p_node_f, symbol_type_array, node);
        p_array_r = list_entry(p_node_r, symbol_type_array, node);
        p_node_r = p_node_r->p_prev;
        if (symbol_type_array_get_size(p_array_f) == 0 || symbol_type_array_get_size(p_array_r) == 0)
            continue;
        if (symbol_type_array_get_size(p_array_f) != symbol_type_array_get_size(p_array_r))
            return false;
    }
    return true;
}

ast_exp::ast_exp(p_symbol_func p_func, p_ast_param_list p_param_list)
{
    assert(p_func);
    // printf("p_func:%s\n", p_func->name);
    this->call = {p_func, p_param_list};
    this->kind = ast_exp_call;
    this->p_type = new symbol_type(p_func->ret_type);

    p_list_head p_node_Fparam = p_func->param.p_next;
    p_list_head p_node;
    list_for_each(p_node, &p_param_list->param)
    {
        if (p_node_Fparam == &p_func->param)
        {
            assert(p_func->is_va);
            break;
        }
        p_symbol_type p_param_type = list_entry(p_node_Fparam, symbol_var, node)->p_type;

        p_ast_param p_param = list_entry(p_node, ast_param, node);
        p_ast_exp p_param_exp = p_param->p_exp;
        if (p_param_type->ref_level == 0 && list_head_alone(&p_param_type->array))
        {
            // printf("cov!!\n");
            p_param_exp = p_param->p_exp = p_param_exp->ast_exp_cov_gen(p_param_type->basic);
        }
        assert(param_arr_check(p_param_type, p_param_exp->p_type));
        p_node_Fparam = p_node_Fparam->p_next;
    }
    assert(p_node_Fparam == &p_func->param);
}
ast_exp::ast_exp(p_symbol_var p_var)
{
    assert(p_var);
    this->p_var = p_var;
    this->kind = ast_exp_ptr;
    this->p_type = p_var->p_type->symbol_type_copy();
    this->p_type->symbol_type_push_ptr();
    // printf("when gen::%lu\n", this->p_type->ref_level);
}
ast_exp::ast_exp(p_ast_exp p_val, p_ast_exp p_offset, bool is_element)
{
    assert(p_val->p_type->ref_level == 1);
    assert(!is_element || !list_head_alone(&p_val->p_type->array));
    p_offset = exp_ptr_to_val_check_basic(p_offset);
    this->gep = {is_element, p_val->ast_exp_ptr_is_stack(), p_val, p_offset};
    this->kind = ast_exp_gep;
    this->p_type = p_val->p_type->symbol_type_copy();
    if (is_element)
    {
        // maybe problem
        this->p_type->symbol_type_pop_ptr();
        delete (this->p_type->symbol_type_pop_array());
        this->p_type->symbol_type_push_ptr();
    }
    // printf("@@@\n");
    // printf("fffff   %lu\n", this->p_type->ref_level);
}
ast_exp::ast_exp(p_ast_exp p_ptr)
{
    assert(p_ptr->p_type->ref_level > 0);
    if (p_ptr->p_type->ref_level == 1)
    {
        assert(list_head_alone(&p_ptr->p_type->array));
    }

    this->load = {p_ptr->ast_exp_ptr_is_stack(), p_ptr};
    this->kind = ast_exp_load;
    this->p_type = p_ptr->p_type->symbol_type_copy();
}
p_ast_exp ast_exp::ast_exp_load_gen()
{
    p_ast_exp re = new ast_exp(this);
    re->p_type->symbol_type_pop_ptr();
    return exp_val_const(re);
}
ast_exp::ast_exp(I32CONST_t num)
{
    this->i32const = num;
    this->kind = ast_exp_num;
    this->p_type = new symbol_type(type_i32);
}
ast_exp::ast_exp(F32CONST_t num)
{
    this->f32const = num;
    this->kind = ast_exp_num;
    this->p_type = new symbol_type(type_f32);
}
ast_exp::ast_exp(p_symbol_str p_str)
{
    this->p_str = p_str;
    this->kind = ast_exp_num;
    this->p_type = new symbol_type(type_str);
}
ast_exp::ast_exp(ast_exp_binary_op b_op, p_ast_exp p_src_1, p_ast_exp p_src_2)
{
    this->b = {p_src_1, p_src_2, b_op};
    this->kind = ast_exp_binary;
    this->p_type = new symbol_type(p_src_1->p_type->basic);
}
p_ast_exp ast_exp_binary_gen(ast_exp_binary_op b_op, p_ast_exp p_src_1, p_ast_exp p_src_2)
{
    assert(p_src_1 && p_src_2);
    p_src_1 = exp_ptr_to_val_check_basic(p_src_1);
    p_src_2 = exp_ptr_to_val_check_basic(p_src_2);
    if (b_op == ast_exp_op_mod)
        assert(p_src_1->p_type->basic == type_i32 && p_src_2->p_type->basic == type_i32);

    if (p_src_1->p_type->basic == type_f32)
    {
        p_src_2 = p_src_2->ast_exp_cov_gen(type_f32);
    }
    else if (p_src_2->p_type->basic == type_f32)
    {
        p_src_1 = p_src_1->ast_exp_cov_gen(type_f32);
    }
    basic_type type = p_src_1->p_type->basic;

    if (p_src_1->kind == ast_exp::ast_exp_num && p_src_2->kind == ast_exp::ast_exp_num)
    {
        switch (b_op)
        {
        case ast_exp_op_add:
            if (type == type_i32)
            {
                p_src_1->i32const = p_src_1->i32const + p_src_2->i32const;
            }
            else if (type == type_f32)
            {
                p_src_1->f32const = p_src_1->f32const + p_src_2->f32const;
            }
            break;
        case ast_exp_op_sub:
            if (type == type_i32)
            {
                p_src_1->i32const = p_src_1->i32const - p_src_2->i32const;
            }
            else if (type == type_f32)
            {
                p_src_1->f32const = p_src_1->f32const - p_src_2->f32const;
            }
            break;
        case ast_exp_op_mul:
            if (type == type_i32)
            {
                p_src_1->i32const = p_src_1->i32const * p_src_2->i32const;
            }
            else if (type == type_f32)
            {
                p_src_1->f32const = p_src_1->f32const * p_src_2->f32const;
            }
            break;
        case ast_exp_op_div:
            if (type == type_i32)
            {
                p_src_1->i32const = p_src_1->i32const / p_src_2->i32const;
            }
            else if (type == type_f32)
            {
                p_src_1->f32const = p_src_1->f32const / p_src_2->f32const;
            }
            break;
        case ast_exp_op_mod:
            p_src_1->i32const = p_src_1->i32const % p_src_2->i32const;
            break;
        default:
            assert(1);
        }
        delete (p_src_2);
        return p_src_1;
    }

    return new ast_exp((ast_exp_binary_op)b_op, p_src_1, p_src_2);
}
ast_exp::ast_exp(ast_exp_unary_op u_op, p_ast_exp p_src)
{
    this->u = {p_src, u_op};
    this->kind = ast_exp_unary;
    this->p_type = new symbol_type(p_src->p_type->basic);
}
p_ast_exp ast_exp_unary_gen(ast_exp_unary_op u_op, p_ast_exp p_src)
{
    assert(p_src);
    p_src = exp_ptr_to_val_check_basic(p_src);

    // basic_type type = p_src->p_type->basic;

    if (p_src->kind == ast_exp::ast_exp_num)
    {
        switch (u_op)
        {
        case ast_exp_op_minus:
            if (p_src->p_type->basic == type_i32)
                p_src->i32const = -p_src->i32const;
            else if (p_src->p_type->basic == type_f32)
                p_src->f32const = -p_src->f32const;
            break;
        default:
            assert(1);
        }
        return p_src;
    }
    return new ast_exp((ast_exp_unary_op)u_op, p_src);
}
ast_exp::~ast_exp()
{
    p_ast_exp p_exp = this;
    assert(p_exp);
    switch (p_exp->kind)
    {
    case ast_exp_binary:
        delete (p_exp->b.p_src_1);
        delete (p_exp->b.p_src_2);
        break;
    case ast_exp_relational:
        delete (p_exp->r.p_rsrc_1);
        delete (p_exp->r.p_rsrc_2);
        break;
    case ast_exp_unary:
        delete (p_exp->u.p_src);
        break;
    case ast_exp_logic:
        delete (p_exp->l.p_bool_1);
        delete (p_exp->l.p_bool_2);
        break;
    case ast_exp_ulogic:
        delete (p_exp->ul.p_bool);
        break;
    case ast_exp_call:
        delete (p_exp->call.p_param_list);
        break;
    case ast_exp_gep:
        delete (p_exp->gep.p_addr);
        delete (p_exp->gep.p_offset);
        break;
    case ast_exp_load:
        delete (p_exp->load.p_ptr);
        break;
    case ast_exp_ptr:
    case ast_exp_num:
    case ast_exp_use:
        break;
    }
    delete (p_exp->p_type);
}

p_ast_exp ast_exp::ast_exp_use_gen()
{
    p_ast_exp re = new ast_exp();
    re->p_exp = this;
    re->kind = ast_exp_use;
    re->p_type = this->p_type->symbol_type_copy();
    return re;
}

void ast_exp::ast_exp_ptr_check_lval()
{
    assert(list_head_alone(&this->p_type->array));
    assert(this->p_type->ref_level == 1);
    assert(this->p_type->basic != type_void);
}
p_ast_exp ast_exp::ast_exp_ptr_to_val_check_basic()
{
    return exp_ptr_to_val_check_basic(this);
}
p_ast_exp ast_exp::ast_exp_ptr_to_val()
{
    // printf("ast_exp_ptr_to_val\n");
    return exp_ptr_to_val(this);
}
bool ast_exp::ast_exp_ptr_is_stack()
{
    p_ast_exp p_tmp = this;
    while (p_tmp->kind == ast_exp_use)
    {
        p_tmp = p_tmp->p_exp;
    }
    if (p_tmp->kind == ast_exp_gep)
    {
        return p_tmp->gep.is_stack_for_gep;
    }
    if (p_tmp->kind == ast_exp_load)
    {
        return false;
    }
    assert(p_tmp->kind == ast_exp_ptr);
    if (p_tmp->p_var->is_global)
        return false;
    return true;
}
static inline p_ast_exp ast_exp_i2f_gen(p_ast_exp p_i32)
{
    assert(p_i32);
    p_i32 = exp_ptr_to_val_check_basic(p_i32);
    assert(p_i32->p_type->basic == type_i32);

    if (p_i32->kind == ast_exp::ast_exp_num)
    {
        p_i32->p_type->basic = type_f32;
        p_i32->f32const = p_i32->i32const;
        return p_i32;
    }

    return new ast_exp((ast_exp_unary_op)ast_exp_op_i2f, p_i32);
}

static inline p_ast_exp ast_exp_f2i_gen(p_ast_exp p_f32)
{
    assert(p_f32);
    p_f32 = exp_ptr_to_val_check_basic(p_f32);
    assert(p_f32->p_type->basic == type_f32);

    if (p_f32->kind == ast_exp::ast_exp_num)
    {
        p_f32->p_type->basic = type_i32;
        p_f32->i32const = p_f32->f32const;
        return p_f32;
    }

    return new ast_exp((ast_exp_unary_op)ast_exp_op_f2i, p_f32);
}
p_ast_exp ast_exp::ast_exp_cov_gen(basic_type b_type)
{

    exp_check_basic(this);
    if (this->p_type->basic != b_type)
    {
        if (this->p_type->basic == type_i32)
        {
            return ast_exp_i2f_gen(this);
        }
        else if (this->p_type->basic == type_f32)
        {
            return ast_exp_f2i_gen(this);
        }
    }
    return this;
}
p_ast_exp ast_exp::ast_exp_to_cond()
{
    p_ast_exp p_exp = this;
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();

    if (p_exp->kind == ast_exp_relational)
        return p_exp;
    if (p_exp->kind == ast_exp_logic)
        return p_exp;
    if (p_exp->kind == ast_exp_ulogic)
        return p_exp;
    return new ast_exp(ast_exp_op_neq, p_exp, new ast_exp((I32CONST_t)0));
}

p_ast_exp ast_exp_ptr_check_const(p_ast_exp p_exp)
{
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
    assert(p_exp->kind == ast_exp::ast_exp_num);
    return p_exp;
}

p_ast_exp syntax_val_offset(p_ast_exp p_val, p_ast_exp p_offset)
{
    // printf("refl::%lu\n", p_val->p_type->ref_level);
    // printf("offset::%d\n", p_offset->i32const);
    if (p_val->p_type->ref_level > 1)
    {
        p_val = p_val->ast_exp_load_gen();
        return new ast_exp(p_val, p_offset, false);
    }
    else
    {
        return new ast_exp(p_val, p_offset, true);
    }
}