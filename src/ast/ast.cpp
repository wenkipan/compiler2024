#include <ast/ast.hpp>

// ast_block
ast_block::ast_block()
{
    length = 0;
    stmt = list_head_init(&this->stmt);
}
p_ast_block ast_block::ast_block_add(p_ast_stmt p_stmt)
{
    if (p_stmt->type != ast_stmt::ast_stmt_block)
    {
        if (p_stmt->type == ast_stmt::ast_stmt_exp && p_stmt->branch.p_exp == NULL)
        {
            // todo need change
            free(p_stmt);
        }
        else
        {
            ++this->length;
            list_add_prev(&p_stmt->node, &this->stmt);
        }
    }
    else
    {
        this->length += p_stmt->p_block->length;
        if (!list_blk_add_prev(&p_stmt->p_block->stmt, &this->stmt))
            list_replace(&this->stmt, &p_stmt->p_block->stmt);
        // todo need change
        free(p_stmt->p_block);
        free(p_stmt);
    }
    return this;
}
ast_block::~ast_block()
{
    assert(this);
    while (!list_head_alone(&this->stmt))
    {
        p_ast_stmt p_stmt = list_entry(this->stmt.p_next, ast_stmt, node);
        // todo need change
        delete (p_stmt);
    }
    // free(this);
}

// support
static inline p_ast_exp exp_ptr_to_val(p_ast_exp p_exp)
{
    printf("0\n");
    if (p_exp->p_type->ref_level == 0)
    {
        assert(list_head_alone(&p_exp->p_type->array));
        return p_exp;
    }
    printf("1\n");
    if (p_exp->p_type->ref_level > 1)
    {
        // return ast_exp_load_gen(p_exp)
        return new ast_exp(p_exp); // load
    }
    printf("2\n");
    // assert(p_exp->p_type->ref_level == 1);
    if (list_head_alone(&p_exp->p_type->array))
    {
        return new ast_exp(p_exp); // load
    }
    printf("3\n");
    return new ast_exp(p_exp, new ast_exp((I32CONST_t)0), true); // gep(//int)
}
static inline void exp_check_basic(p_ast_exp p_exp)
{
    assert(list_head_alone(&p_exp->p_type->array) && p_exp->p_type->ref_level == 0);
    assert(p_exp->p_type->basic != type_void);
}
static inline p_ast_exp exp_ptr_to_val_check_basic(p_ast_exp p_exp)
{
    p_exp = exp_ptr_to_val(p_exp);
    exp_check_basic(p_exp);
    return p_exp;
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

// class ast_exp
// ast_exp::ast_exp(p_ast_exp p_used_exp)
// {
//     assert(p_used_exp);
//     // p_ast_exp p_exp = malloc(sizeof(*p_exp));
//     // *p_exp = (ast_exp){
//     //     .kind = ast_exp_use,
//     //     .p_exp = p_used_exp,
//     //     .p_type = symbol_type_copy(p_used_exp->p_type),
//     //     .p_des = NULL,
//     // };
//     // return p_exp;
//     kind = ast_exp_use;
//     p_exp = p_used_exp;
//     p_type = p_used_exp->p_type->symbol_type_copy();
//     ////p_des = NULL;
// }
ast_exp::ast_exp(ast_exp_relational_op r_op, p_ast_exp p_rsrc_1, p_ast_exp p_rsrc_2)
{
    assert(p_rsrc_1 && p_rsrc_2);
    p_rsrc_1 = exp_ptr_to_val_check_basic(p_rsrc_1);
    p_rsrc_2 = exp_ptr_to_val_check_basic(p_rsrc_2);

    if (p_rsrc_1->p_type->basic == type_f32)
    {
        p_rsrc_2->ast_exp_cov_gen(type_f32);
    }
    else if (p_rsrc_2->p_type->basic == type_f32)
    {
        p_rsrc_1->ast_exp_cov_gen(type_f32);
    }

    kind = ast_exp_relational;
    r = {
        .p_rsrc_1 = p_rsrc_1,
        .p_rsrc_2 = p_rsrc_2,
        .r_op = r_op,
    };
    p_type = new symbol_type(type_i32);
    ////p_des = NULL;
}
ast_exp::ast_exp(ast_exp_logic_op l_op, p_ast_exp p_bool_1, p_ast_exp p_bool_2)
{
    assert(p_bool_1 && p_bool_2);
    p_bool_1 = exp_ptr_to_val_check_basic(p_bool_1);
    p_bool_2 = exp_ptr_to_val_check_basic(p_bool_2);

    p_bool_1->ast_exp_to_cond();
    p_bool_2->ast_exp_to_cond();

    kind = ast_exp_logic;
    l = {
        .p_bool_1 = p_bool_1,
        .p_bool_2 = p_bool_2,
        .l_op = l_op,

    };
    p_type = new symbol_type(type_i32);
    // p_des = NULL;
};
ast_exp::ast_exp(ast_exp_ulogic_op ul_op, p_ast_exp p_bool)
{
    assert(p_bool);
    p_bool = exp_ptr_to_val_check_basic(p_bool);

    p_bool->ast_exp_to_cond();

    kind = ast_exp_ulogic;
    ul = {
        .p_bool = p_bool,
        .ul_op = ul_op,
    };
    p_type = new symbol_type(type_i32);
    // p_des = NULL;
}
ast_exp::ast_exp(p_symbol_func p_func, p_ast_param_list p_param_list)
{
    assert(p_func);
    kind = ast_exp_call;
    call = {
        .p_func = p_func,
        .p_param_list = p_param_list,

    };
    p_type = new symbol_type(p_func->ret_type);
    // p_des = NULL;

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
            // todo
            p_param->p_exp = p_param_exp->ast_exp_cov_gen(p_param_type->basic);
        }
        assert(param_arr_check(p_param_type, p_param_exp->p_type));
        p_node_Fparam = p_node_Fparam->p_next;
    }
    assert(p_node_Fparam == &p_func->param);
}
ast_exp::ast_exp(p_symbol_var p_var)
{
    assert(p_var);
    kind = ast_exp_ptr;
    this->p_var = p_var;
    this->p_type = p_var->p_type->symbol_type_copy();
    // p_des = NULL;
    this->p_type->symbol_type_push_ptr();
}
ast_exp::ast_exp(p_ast_exp p_val, p_ast_exp p_offset, bool is_element)
{
    assert(p_val->p_type->ref_level == 1);
    assert(!is_element || !list_head_alone(&p_val->p_type->array));
    p_offset = exp_ptr_to_val_check_basic(p_offset);
    kind = ast_exp_gep;
    gep = {
        .is_element = is_element,
        .is_stack_for_gep = p_val->ast_exp_ptr_is_stack(),
        .p_addr = p_val,
        .p_offset = p_offset,
    };
    p_type = p_val->p_type->symbol_type_copy();
    // p_des = NULL;
    if (is_element)
    {
        // maybe problem
        // symbol_type_pop_ptr(p_exp->p_type);
        // symbol_type_array_drop(symbol_type_pop_array(p_exp->p_type));
        // symbol_type_push_ptr(p_exp->p_type);
        p_exp->p_type->symbol_type_pop_ptr();
        delete (p_exp->p_type->symbol_type_pop_array());
        p_exp->p_type->symbol_type_push_ptr();
    }
}
ast_exp::ast_exp(p_ast_exp p_ptr)
{
    assert(p_ptr->p_type->ref_level > 0);
    if (p_ptr->p_type->ref_level == 1)
    {
        assert(list_head_alone(&p_ptr->p_type->array));
    }
    kind = ast_exp_load;
    load = {
        .is_stack = p_ptr->ast_exp_ptr_is_stack(),
        .p_ptr = p_ptr,

    };
    p_type = p_ptr->p_type->symbol_type_copy();
    // p_des = NULL;
    p_exp->p_type->symbol_type_pop_ptr();
}
ast_exp::ast_exp(I32CONST_t num)
{
    kind = ast_exp_num;
    i32const = num;
    p_type = new symbol_type(type_i32);
    // p_des = NULL;
}
ast_exp::ast_exp(F32CONST_t num)
{
    kind = ast_exp_num;
    f32const = num;
    p_type = new symbol_type(type_f32);
    ////p_des = NULL;
}
ast_exp::ast_exp(p_symbol_str p_str)
{
    kind = ast_exp_num;
    this->p_str = p_str;
    p_type = new symbol_type(type_str);
    ////p_des = NULL;
}
ast_exp::ast_exp(ast_exp_binary_op b_op, p_ast_exp p_src_1, p_ast_exp p_src_2)
{
    basic_type type = p_src_1->p_type->basic;
    kind = ast_exp::ast_exp_binary;
    b = {
        .p_src_1 = p_src_1,
        .p_src_2 = p_src_2,
        .b_op = b_op,

    };
    p_type = new symbol_type(type);
    ////p_des = NULL;
}
ast_exp::ast_exp(ast_exp_unary_op u_op, p_ast_exp p_src)
{
    basic_type type = p_src->p_type->basic;
    kind = ast_exp_unary;
    u = {
        .p_src = p_src,
        .u_op = u_op,
    };
    p_type = new symbol_type(type);
    // p_des = NULL;
}
ast_exp::~ast_exp()
{
    assert(this);
    switch (this->kind)
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
    // free(p_exp);
}
// other number func
p_ast_exp ast_exp::ast_exp_use_gen()
{
    assert(this);
    p_ast_exp p_exp = new ast_exp();
    p_exp->kind = ast_exp::ast_exp_use;
    p_exp->p_exp = this;
    p_exp->p_type = this->p_type->symbol_type_copy();
    // p_exp->//p_des = NULL;
    return p_exp;
}

void ast_exp::ast_exp_ptr_check_lval()
{
    assert(list_head_alone(&this->p_type->array));
    assert(this->p_type->ref_level == 1);
    assert(this->p_type->basic != type_void);
}
// p_ast_exp ast_exp::ast_exp_ptr_check_const()
// {
//     this = this->ast_exp_ptr_to_val_check_basic();
//     assert(this->kind == ast_exp_num);
//     return this;
// }
p_ast_exp ast_exp_ptr_check_const(p_ast_exp p_exp)
{
    p_exp = p_exp->ast_exp_ptr_to_val_check_basic();
    assert(p_exp->kind == ast_exp::ast_exp_num);
    return p_exp;
}
p_ast_exp ast_exp::ast_exp_ptr_to_val_check_basic()
{
    return exp_ptr_to_val_check_basic(this);
}
p_ast_exp ast_exp::ast_exp_ptr_to_val()
{
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

    p_ast_exp p_exp = new ast_exp();
    p_exp->kind = ast_exp::ast_exp_unary;
    p_exp->u.u_op = ast_exp_op_i2f;
    p_exp->u.p_src = p_i32;
    p_exp->p_type = new symbol_type(type_f32);
    // p_exp->//p_des = NULL;
    return p_exp;
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

    p_ast_exp p_exp = new ast_exp();
    p_exp->kind = ast_exp::ast_exp_unary;
    p_exp->u.u_op = ast_exp_op_f2i;
    p_exp->u.p_src = p_f32;
    p_exp->p_type = new symbol_type(type_f32);
    // p_exp->//p_des = NULL;
    return p_exp;
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
        else if (p_exp->p_type->basic == type_f32)
        {
            return ast_exp_f2i_gen(this);
        }
    }
    return this;
}
p_ast_exp ast_exp::ast_exp_to_cond()
{
    p_ast_exp p_exp = this;
    this->ast_exp_ptr_to_val_check_basic();
    if (this->kind == ast_exp_relational)
        return this;
    if (this->kind == ast_exp_logic)
        return this;
    if (this->kind == ast_exp_ulogic)
        return this;
    return new ast_exp(ast_exp_op_neq, p_exp, new ast_exp((I32CONST_t)0)); // relation(int)
}

p_ast_exp syntax_val_offset(p_ast_exp p_val, p_ast_exp p_offset)
{
    if (p_val->p_type->ref_level > 1)
    {
        p_val = new ast_exp(p_val);
        return new ast_exp(p_val, p_offset, false);
    }
    else
    {
        return new ast_exp(p_val, p_offset, true);
    }
}

// hard to change
p_ast_exp ast_exp_binary_gen(ast_exp_binary_op b_op, p_ast_exp p_src_1, p_ast_exp p_src_2)
{
    assert(p_src_1 && p_src_2);
    p_src_1 = exp_ptr_to_val_check_basic(p_src_1);
    p_src_2 = exp_ptr_to_val_check_basic(p_src_2);
    if (b_op == ast_exp_op_mod)
        assert(p_src_1->p_type->basic == type_i32 && p_src_2->p_type->basic == type_i32);

    if (p_src_1->p_type->basic == type_f32)
    {
        p_src_2->ast_exp_cov_gen(type_f32);
    }
    else if (p_src_2->p_type->basic == type_f32)
    {
        p_src_1->ast_exp_cov_gen(type_f32);
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
    // todo
    p_ast_exp p_exp = new ast_exp((ast_exp_binary_op)b_op, p_src_1, p_src_2);
    return p_exp;
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
    p_ast_exp p_exp = new ast_exp((ast_exp_unary_op)u_op, p_src);
    return p_exp;
}

// ast_param_list begin
ast_param_list::ast_param_list()
{
    // maybe problem
    param = list_head_init(&this->param);
}
p_ast_param_list ast_param_list::ast_param_list_add(p_ast_exp p_exp)
{
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
        delete (p_param);
    }
}
// ast_param
ast_param::ast_param(p_ast_exp p_exp)
{
    p_exp = p_exp->ast_exp_ptr_to_val();
    this->p_exp = p_exp;
    is_stck_ptr = p_exp->p_type->ref_level > 0 ? p_exp->ast_exp_ptr_is_stack() : false;
    node = list_head_init(&this->node);
}
ast_param::~ast_param()
{
    assert(this);
    list_del(&this->node);
    delete (this->p_exp);
}

// ast_stmt begin
p_ast_stmt ast_stmt_return_gen(basic_type ret_type, p_ast_exp p_exp)
{
    if (p_exp)
    {
        p_exp->ast_exp_ptr_to_val_check_basic();
        p_exp->ast_exp_cov_gen(ret_type);
    }
    else
    {
        assert(ret_type == type_void);
    }

    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_return,
        .branch = {
            .p_exp = p_exp,
        },
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_exp_gen(p_ast_exp p_exp)
{
    if (p_exp)
        p_exp = p_exp->ast_exp_ptr_to_val();
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_exp,
        .branch{
            .p_exp = p_exp,
        },
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_break_gen(void)
{
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_break,
        .null = NULL,
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_continue_gen(void)
{
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_continue,
        .null = NULL,
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_if_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1)
{
    assert(p_exp && p_stmt_1);
    p_exp->ast_exp_ptr_to_val_check_basic();
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_if,
        .branch{
            .p_exp = p_exp,
            .p_stmt_1 = p_stmt_1,
        },
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_if_else_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2)
{
    assert(p_exp && p_stmt_1 && p_stmt_2);
    p_exp->ast_exp_ptr_to_val_check_basic();
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_if_else,
        .branch{
            .p_exp = p_exp,
            .p_stmt_1 = p_stmt_1,
            .p_stmt_2 = p_stmt_2,
        },
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1)
{
    assert(p_exp && p_stmt_1);
    p_exp->ast_exp_ptr_to_val_check_basic();
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_while,
        .branch{
            .p_exp = p_exp,
            .p_stmt_1 = p_stmt_1,
        },
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}

p_ast_stmt ast_stmt_block_gen(p_ast_block p_block)
{
    assert(p_block);
    if (p_block->length == 0)
    {
        free(p_block);
        return ast_stmt_exp_gen(NULL);
    }
    if (p_block->length == 1)
    {
        p_ast_stmt p_stmt = list_entry(p_block->stmt.p_next, ast_stmt, node);
        list_del(&p_stmt->node);
        free(p_block);
        return p_stmt;
    }
    p_ast_stmt p_stmt = new ast_stmt();
    *p_stmt = (ast_stmt){
        .type = ast_stmt::ast_stmt_block,
        .p_block = p_block,
        .node = list_head_init(&p_stmt->node),
    };
    return p_stmt;
}
p_ast_stmt ast_stmt_assign_gen(p_ast_exp lval, p_ast_exp rval)
{
    // maybe problem
    assert(lval && rval);
    lval->ast_exp_ptr_check_lval();
    rval = rval->ast_exp_ptr_to_val_check_basic();
    rval = rval->ast_exp_cov_gen(lval->p_type->basic);

    p_ast_stmt p_exp = new ast_stmt();
    *p_exp = {
        .type = ast_stmt::ast_stmt_assign,
        .array{
            .is_stack = lval->ast_exp_ptr_is_stack(),
            .p_lval = lval,
            .p_rval = rval,
        },
        .node = list_head_init(&p_exp->node),
    };
    return p_exp;
}

ast_stmt::~ast_stmt()
{
    assert(this);
    list_del(&this->node);
    switch (this->type)
    {
    case ast_stmt_assign:
        delete (this->array.p_lval);
        delete (this->array.p_rval);
        break;
    case ast_stmt_block:
        delete (this->p_block);
        break;
    case ast_stmt_exp:
        if (this->branch.p_exp)
            delete (this->branch.p_exp);
        break;
    case ast_stmt_return:
        if (this->branch.p_exp)
            delete (this->branch.p_exp);
        break;
    case ast_stmt_if_else:
        delete (this->branch.p_exp);
        delete (this->branch.p_stmt_1);
        delete (this->branch.p_stmt_2);
        break;
    case ast_stmt_while:
        delete (this->branch.p_exp);
        delete (this->branch.p_stmt_1);
        break;
    case ast_stmt_if:
        delete (this->branch.p_exp);
        delete (this->branch.p_stmt_1);
        break;
    case ast_stmt_break:
    case ast_stmt_continue:
        break;
    }
}