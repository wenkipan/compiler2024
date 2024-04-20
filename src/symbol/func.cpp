#include <symbol/func.hpp>
#include <symbol/symbol.hpp>
symbol_func::symbol_func(const char *name, basic_type b_type, bool is_va)
    : is_va(is_va),
      ret_type(b_type),
      name(new char[strlen(name) + 1]),
      id(0),
      var_cnt(0),
      param(list_head_init(&this->param)),
      variable(list_head_init(&this->variable)),
      param_reg_cnt(0),
      param_reg_list(list_head_init(&this->param_reg_list)),
      vreg_cnt(0),
      vreg_list(list_head_init(&this->vreg_list)),
      block_cnt(0),
      block(list_head_init(&this->block)),
      call_param_vmem_list(list_head_init(&this->call_param_vmem_list)),
      stack_size(0),
      inner_stack_size(0),
      instr_num(0),
      node(list_head_init(&this->node)),
      if_updated_graph(true)
{
    strcpy(this->name, name);
}

void symbol_func::symbol_func_add_variable(p_symbol_var p_var)
{
    p_var->id = this->var_cnt++;
    list_add_prev(&p_var->node, &this->variable);
}

void symbol_func::symbol_func_add_call_param_vmem(p_symbol_var p_vmem)
{
    p_symbol_func p_func = this;
    p_vmem->id = p_func->var_cnt++;
    if (p_vmem->stack_offset + basic_type_get_size(p_vmem->p_type->basic) > p_func->inner_stack_size)
        p_func->inner_stack_size = p_vmem->stack_offset + basic_type_get_size(p_vmem->p_type->basic);
    list_add_prev(&p_vmem->node, &p_func->call_param_vmem_list);
}

void symbol_func::symbol_func_add_param(p_symbol_var p_var)
{
    p_var->id = this->var_cnt++;
    list_add_prev(&p_var->node, &this->param);
}

void symbol_func::symbol_func_init_print()
{
    p_symbol_func p_func = this;
    printf("define ");
    symbol_basic_type_print(p_func->ret_type);
    printf(" ");
    p_func->symbol_func_name_print();
    printf(" (");
    p_list_head p_node;
    list_for_each(p_node, &p_func->param)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        p_var->symbol_name_print();
        if (p_node != p_func->param.p_prev)
            printf(", ");
    }
    printf(")\n");
}

void symbol_func::symbol_func_name_print()
{
    printf("%s", this->name);
}

// void symbol_func::symbol_func_add_block(p_ast_block p_block)
// {
//     this->block = p_block->stmt;
// }

// relative to ir dont do now
symbol_func::~symbol_func()
{
    p_symbol_func p_func = this;
    list_del(&p_func->node);
    while (!list_head_alone(&p_func->param))
    {
        p_symbol_var p_del = list_entry(p_func->param.p_next, symbol_var, node);
        delete (p_del);
    }
    while (!list_head_alone(&p_func->variable))
    {
        p_symbol_var p_del = list_entry(p_func->variable.p_next, symbol_var, node);
        delete (p_del);
    }
    while (!list_head_alone(&p_func->call_param_vmem_list))
    {
        p_symbol_var p_del = list_entry(p_func->call_param_vmem_list.p_next, symbol_var, node);
        delete (p_del);
    }
    assert(p_func->block_cnt == 0);
    assert(p_func->instr_num == 0);
    assert(p_func->vreg_cnt == 0);
    assert(p_func->param_reg_cnt == 0);
    delete[] this->name;
}