#include <ast2ir/ast2ir.hpp>
#include <new>
void ast2ir::ast2ir_add_function(p_symbol_func p_func, p_ast_block p_h_block)
{
    Function *p_ir_fun = new Function;
    module->functions_push_back(p_ir_fun);

    function_add_params(p_func);

    function_add_params();
    function_add_retval();
    functon_add_block();
}

void ast2ir::function_add_params(p_symbol_func p_func)
{
    p_list_head p_node;
    list_for_each(p_node, &p_func->param)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);

        module->get_functions()->back();

        p_var->p_type->basic;
        symbol_func_param_reg_add(p_func, p_param);
        ast2ir_info_add_instr(p_info, ir_store_instr_gen(ir_operand_addr_gen(p_var, NULL, 0), ir_operand_vreg_gen(p_param), true));
    }
}