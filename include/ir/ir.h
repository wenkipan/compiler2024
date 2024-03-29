#pragma once

#include <util.h>
#include <symbol.h>
typedef class ir_basic_block_branch *p_ir_basic_block_branch;
typedef class ir_basic_block *p_ir_basic_block;
typedef class ir_basic_block_branch_target *p_ir_basic_block_branch_target;
typedef class ir_branch_target_node *p_ir_branch_target_node;
typedef class ir_bb_param *p_ir_bb_param;
typedef class ir_bb_phi *p_ir_bb_phi;
typedef class ir_vreg_list *p_ir_vreg_list;
typedef class ir_vreg_list_node *p_ir_vreg_list_node;

typedef class ir_basic_block_list *p_ir_basic_block_list;
typedef class ir_basic_block_list_node *p_ir_basic_block_list_node;

typedef class ir_param *p_ir_param;
typedef class ir_param_list *p_ir_param_list;

typedef class ir_operand *p_ir_operand;

typedef class ir_vreg *p_ir_vreg;

typedef class ir_instr *p_ir_instr;

typedef class ir_binary_instr *p_ir_binary_instr;
typedef class ir_unary_instr *p_ir_unary_instr;
typedef class ir_alloca_instr *p_ir_alloca_instr;
typedef class ir_gep_instr *p_ir_gep_instr;
typedef class ir_load_instr *p_ir_load_instr;
typedef class ir_store_instr *p_ir_store_instr;
typedef class ir_call_instr *p_ir_call_instr;
typedef class ir_ret_instr *p_ir_ret_instr;
typedef class ir_br_instr *p_ir_br_instr;
typedef class ir_condbr_instr *p_ir_condbr_instr;