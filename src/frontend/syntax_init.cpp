
#include <frontend/syntax_init.hpp>

syntax_init::syntax_init()
    : is_exp(false),
      syntax_const(true),
      list(list_head_init(&this->list)),
      node(list_head_init(&this->node))
{
}
syntax_init::syntax_init(p_ast_exp p_exp)
    : is_exp(true),
      syntax_const(p_exp->kind == ast_exp::ast_exp_num),
      p_exp(p_exp),
      node(list_head_init(&this->node))
{
}

syntax_init::~syntax_init()
{
    // if (!this)
    //     return;

    if (this->is_exp)
    {
        // ast_exp_drop(p_init->p_exp);
        // free(p_init);
        return;
    }

    p_list_head p_node, p_next;
    list_for_each_safe(p_node, p_next, &this->list)
    {
        p_syntax_init p_del = list_entry(p_node, syntax_init, node);
        list_del(p_node);
        delete (p_del);
    }
}

p_syntax_init syntax_init::syntax_init_list_add(p_syntax_init p_init)
{
    assert(!this->is_exp);
    if (!p_init->syntax_const)
        this->syntax_const = false;
    assert(list_add_prev(&p_init->node, &this->list));
    return this;
}
static inline p_list_head syntax_init_list_regular(p_list_head p_list, p_list_head p_node, p_symbol_type p_type)
{
    if (!p_node)
    {
        p_node = p_list->p_next;
    }

    assert(!list_head_alone(&p_type->array));
    p_symbol_type_array p_pop = p_type->symbol_type_pop_array();
    size_t cnt = symbol_type_array_get_size(p_pop);
    bool is_basic = list_head_alone(&p_type->array);

    for (size_t i = 0; i < cnt; ++i)
    {
        if (p_node == p_list)
            break;
        p_syntax_init p_init = list_entry(p_node, syntax_init, node);

        if (is_basic)
        {
            assert(p_init->is_exp);
            p_ast_exp p_init_exp = p_init->p_exp->ast_exp_ptr_to_val_check_basic();
            p_init->p_exp = p_init_exp->ast_exp_cov_gen(p_type->basic);
            p_node = p_node->p_next;
            continue;
        }

        if (!p_init->is_exp)
        {
            syntax_init_list_regular(&p_init->list, NULL, p_type);
            p_node = p_node->p_next;
            continue;
        }

        p_syntax_init p_ins = new syntax_init();
        list_add_prev(&p_ins->node, p_node);
        p_list_head p_end = syntax_init_list_regular(p_list, p_node, p_type);
        p_list_head p_next;
        for (p_node = p_ins->node.p_next, p_next = p_node->p_next; p_node != p_end; p_node = p_next, p_next = p_node->p_next)
        {
            list_del(p_node);
            list_add_prev(p_node, &p_ins->list);
        }
    }

    p_pop->symbol_type_push_array(p_type);
    return p_node;
}

p_syntax_init syntax_init::syntax_init_regular(p_symbol_type p_type)
{
    p_syntax_init p_init = this;
    if (!p_init)
        return NULL;

    if (!p_init->is_exp)
    {
        syntax_init_list_regular(&p_init->list, NULL, p_type);
    }
    else
    {
        assert(list_head_alone(&p_type->array));
        p_ast_exp p_init_exp = p_init->p_exp->ast_exp_ptr_to_val_check_basic();
        p_init->p_exp = p_init_exp->ast_exp_cov_gen(p_type->basic);
    }

    return p_init;
}

p_list_head syntax_init::syntax_init_get_head()
{
    assert(!this->is_exp);
    return &this->list;
}
p_ast_exp syntax_init::syntax_init_get_exp()
{
    assert(this->is_exp);
    return this->p_exp;
}

p_ast_exp syntax_init::syntax_init_find_exp(p_symbol_type p_type, size_t offset)
{
    p_syntax_init p_init = this;
    if (!p_init)
        return NULL;

    assert(offset < p_type->size);
    if (list_head_alone(&p_type->array))
    {
        assert(p_init->is_exp);
        return p_init->p_exp;
    }

    p_symbol_type_array p_pop = p_type->symbol_type_pop_array();
    size_t len = p_type->size;
    size_t index = offset / len;

    assert(!p_init->is_exp);
    p_list_head p_node = p_init->list.p_next;
    for (size_t i = 0; p_node != &p_init->list && i < index; ++i)
    {
        p_node = p_node->p_next;
    }

    if (p_node == &p_init->list)
    {
        p_pop->symbol_type_push_array(p_type);
        return NULL;
    }
    p_syntax_init p_inner = list_entry(p_node, syntax_init, node);
    p_ast_exp p_ret = p_inner->syntax_init_find_exp(p_type, offset % len);

    p_pop->symbol_type_push_array(p_type);
    return p_ret;
}

p_syntax_init syntax_init_get_entry(p_list_head p_node)
{
    return list_entry(p_node, syntax_init, node);
}
