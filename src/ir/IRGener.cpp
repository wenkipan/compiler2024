#include <ir/IRGener.hpp>

Module *GenFunction::module = nullptr;
std::unordered_map<p_symbol_var, Value *> *GenFunction::_global_map = new std::unordered_map<p_symbol_var, Value *>();
GenFunction::GenFunction(Function *p_func, BasicBlock *p_block, p_symbol_func _p_func)
    : func(p_func), curBB(p_block), p_ret(nullptr)
{
    if (func->get_type()->get_type() != TypeEnum::Void)
    {
        p_symbol_type _ret_type = new symbol_type((func->get_type()->get_type() == TypeEnum::I32) ? type_i32 : type_f32);
        p_symbol_var _ret_var = new symbol_var("ret", _ret_type, false, false, nullptr);
        p_ret = new Alloca(curBB, _ret_var);
        delete _ret_var;
    }

    retBB = new BasicBlock;
    // param
    p_list_head p_node;
    list_for_each(p_node, &_p_func->param)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        Value *p_alloc = new Alloca(curBB, p_var);
        Param *p_param = new Param(p_alloc->get_type());
        p_func->Param_pushBack(p_param);
        p_func->value_pushBack(p_param);
        _map[p_var] = p_alloc;
        new Store(p_alloc, p_param, true, curBB);
    }

    // variable
    list_for_each(p_node, &_p_func->variable)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        Value *p_alloc = new Alloca(curBB, p_var);
        assert(p_alloc);
        _map[p_var] = p_alloc;
    }
}

GenFunction::~GenFunction()
{
}

void GenFunction::new_curBB()
{
    curBB = func->block_addnewBB();
}

void GenFunction::Func_add_BB(BasicBlock *p_BB)
{
    func->block_pushBack(p_BB);
    set_curBB(p_BB);
}

void GenFunction::stmt2ir(p_ast_block p_block)
{
    p_list_head p_node;
    list_for_each(p_node, &p_block->stmt)
    {
        p_ast_stmt p_stmt = list_entry(p_node, ast_stmt, node);
        ast2ir_stmt_gen(nullptr, nullptr, p_stmt);
    }
    retBB->Set_parent(func);
    func->block_pushBack(retBB);
    func->set_retBB(retBB);
    if (func->get_type()->get_type() != TypeEnum::Void)
    {
        Value *_val = new Load(p_ret, true, retBB);
        new Ret(_val, retBB);
    }
    else
        new Ret(retBB);
}

Value *GenFunction::ast2ir_exp_num_gen(p_ast_exp p_exp)
{
    if (p_exp->p_type->basic == type_i32)
    {
        Value *_ret = new ConstantI32(p_exp->i32const);
        func->value_pushBack(_ret);
        return _ret;
    }
    if (p_exp->p_type->basic == type_f32)
    {
        Value *_ret = new ConstantF32(p_exp->f32const);
        func->value_pushBack(_ret);
        return _ret;
    }
    assert(0);
    if (p_exp->p_type->basic == type_str)
    {
        return new ConstantStr(p_exp->p_str->string);
    }
}

Value *GenFunction::get_addr(p_symbol_var p_var, p_symbol_type p_type, I32CONST_t offset)
{
    if (_map.find(p_var) != _map.end())
    {
        return _map[p_var];
    }
    else
    {
        assert(_global_map->find(p_var) != _global_map->end());
        return (*_global_map)[p_var];
    }
}

Value *GenFunction::ast2ir_exp_gep_gen(p_ast_exp p_exp)
{
    assert(p_exp->gep.p_addr);
    Value *p_addr = ast2ir_exp_gen(p_exp->gep.p_addr);
    Value *p_offset = ast2ir_exp_gen(p_exp->gep.p_offset);
    assert(p_addr->get_type()->get_type() == TypeEnum::Ptr);
    Value *p_gep = new GEP(p_addr, p_offset, p_exp->gep.is_element, curBB);
    p_exp->p_val = p_gep;
    return p_gep;
}

Value *GenFunction::ast2ir_exp_load_gen(p_ast_exp p_exp)
{
    Value *p_ptr = ast2ir_exp_gen(p_exp->load.p_ptr);
    Value *p_load = new Load(p_ptr, p_exp->load.is_stack, curBB);
    p_exp->p_val = p_load;
    return p_load;
}

Value *GenFunction::ast2ir_exp_relational_gen(p_ast_exp p_exp)
{
    assert(p_exp->r.p_rsrc_1 && p_exp->r.p_rsrc_2);
    Value *p_src1 = ast2ir_exp_gen(p_exp->r.p_rsrc_1);
    Value *p_src2 = ast2ir_exp_gen(p_exp->r.p_rsrc_2);
    Value *p_instr = nullptr;
    TypeEnum type1 = p_src1->get_type()->get_type(), type2 = p_src2->get_type()->get_type();
    assert(type1 == TypeEnum::I32 || type1 == TypeEnum::F32);
    assert(type2 == TypeEnum::I32 || type2 == TypeEnum::F32);
    int add = 0;
    if (type1 == TypeEnum::F32 || type2 == TypeEnum::F32)
        add = 6;
    //!!!测试
    InstrutionEnum _type = (InstrutionEnum)(9);
    assert(_type == InstrutionEnum::CmpBegin);
    //!!!
    switch (p_exp->r.r_op)
    {
    case ast_exp_op_eq:
        p_instr = new Cmp((InstrutionEnum)(10 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_neq:
        p_instr = new Cmp((InstrutionEnum)(11 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_g:
        p_instr = new Cmp((InstrutionEnum)(12 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_geq:
        p_instr = new Cmp((InstrutionEnum)(13 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_l:
        p_instr = new Cmp((InstrutionEnum)(14 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_leq:
        p_instr = new Cmp((InstrutionEnum)(15 + add), p_src1, p_src2, curBB);
        break;
    default:
        assert(0);
        break;
    }
    assert(p_instr != nullptr);
    p_exp->p_val = p_instr;
    return p_instr;
}

Value *GenFunction::ast2ir_exp_binary_gen(p_ast_exp p_exp)
{
    assert(p_exp->b.p_src_1 && p_exp->b.p_src_2);
    Value *p_src1 = ast2ir_exp_gen(p_exp->b.p_src_1);
    Value *p_src2 = ast2ir_exp_gen(p_exp->b.p_src_2);
    Value *p_instr = nullptr;
    TypeEnum type1 = p_src1->get_type()->get_type(), type2 = p_src2->get_type()->get_type();
    assert(type1 == TypeEnum::I32 || type1 == TypeEnum::F32);
    assert(type2 == TypeEnum::I32 || type2 == TypeEnum::F32);
    int add = 0;
    if (type1 == TypeEnum::F32 || type2 == TypeEnum::F32)
        add = 5;
    switch (p_exp->b.b_op)
    {
    case ast_exp_op_add:
        p_instr = new Binary((InstrutionEnum)(24 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_sub:
        p_instr = new Binary((InstrutionEnum)(25 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_mul:
        p_instr = new Binary((InstrutionEnum)(26 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_div:
        p_instr = new Binary((InstrutionEnum)(27 + add), p_src1, p_src2, curBB);
        break;
    case ast_exp_op_mod:
        assert(add == 0);
        p_instr = new Binary((InstrutionEnum)(28 + add), p_src1, p_src2, curBB);
        break;
    default:
        assert(0);
        break;
    }
    assert(p_instr);
    p_exp->p_val = p_instr;
    return p_instr;
}

Value *GenFunction::ast2ir_exp_unary_gen(p_ast_exp p_exp)
{
    assert(p_exp->u.p_src);
    Value *p_src1 = ast2ir_exp_gen(p_exp->u.p_src);
    Value *p_instr = nullptr;
    switch (p_exp->u.u_op)
    {
    case ast_exp_op_minus:
        p_instr = new Unary(InstrutionEnum::MINUS, p_src1, curBB);
        break;
    case ast_exp_op_f2i:
        p_instr = new Unary(InstrutionEnum::F2I, p_src1, curBB);
        break;
    case ast_exp_op_i2f:
        p_instr = new Unary(InstrutionEnum::I2F, p_src1, curBB);
        break;
    }
    assert(p_instr);
    p_exp->p_val = p_instr;
    return p_instr;
}

Value *GenFunction::ast2ir_exp_logic_gen(p_ast_exp p_exp)
{
    assert(0);
}

Value *GenFunction::ast2ir_exp_ulogic_gen(p_ast_exp p_exp)
{

    assert(p_exp->ul.p_bool);

    Value *p_instr = nullptr;
    p_ast_exp p_new = p_exp;
    while (p_new->kind == ast_exp::ast_exp_ulogic && p_new->ul.ul_op == ast_exp_op_bool_not)
    {
        p_ast_exp p_inner = p_new->ul.p_bool;
        if (p_inner->kind == ast_exp::ast_exp_ulogic && p_inner->ul.ul_op == ast_exp_op_bool_not)
            p_new = p_inner->ul.p_bool;
        else
            p_new = p_inner;
    }
    if (p_new->kind == ast_exp::ast_exp_logic)
    {
        p_instr = ast2ir_exp_logic_gen(p_new);
        p_exp->p_val = p_new->p_val;
    }

    assert(p_new->kind == ast_exp::ast_exp_relational);
    switch (p_new->r.r_op)
    {
    case ast_exp_op_neq:
        p_new->r.r_op = ast_exp_op_eq;
        break;
    case ast_exp_op_eq:
        p_new->r.r_op = ast_exp_op_neq;
        break;
    case ast_exp_op_g:
        p_new->r.r_op = ast_exp_op_leq;
        break;
    case ast_exp_op_geq:
        p_new->r.r_op = ast_exp_op_l;
        break;
    case ast_exp_op_l:
        p_new->r.r_op = ast_exp_op_geq;
        break;
    case ast_exp_op_leq:
        p_new->r.r_op = ast_exp_op_g;
        break;
    default:
        assert(0);
    }
    p_instr = ast2ir_exp_relational_gen(p_new);
    p_exp->p_val = p_new->p_val;
    return p_instr;
}

Value *GenFunction::ast2ir_exp_call_gen(p_ast_exp p_exp)
{
    std::vector<Function *> *p_funcs = module->get_funcs();
    Value *p_func = nullptr;
    for (Function *p_node : *p_funcs)
    {
        if (p_node->get_name() == std::string(p_exp->call.p_func->name, strlen(p_exp->call.p_func->name)))
            p_func = p_node;
    }
    assert(p_func);
    Value *p_instr = new Call(p_func, curBB);
    curBB->Ins_popBack();
    p_list_head p_node;
    list_for_each(p_node, &p_exp->call.p_param_list->param)
    {
        p_ast_param p_param = list_entry(p_node, ast_param, node);
        Value *p_op = ast2ir_exp_gen(p_param->p_exp);
        if (p_param->is_stck_ptr)
        {
            new Unary(InstrutionEnum::AddSP, p_op, curBB);
        }
        ((Call *)p_instr)->params_pushback(p_op);
    }
    curBB->Ins_pushBack((Instrution *)p_instr);
    p_exp->p_val = p_instr;
    return p_instr;
}

Value *GenFunction::ast2ir_exp_use_gen(p_ast_exp p_exp)
{
    assert(p_exp->p_exp->p_val);
    p_exp->p_val = p_exp->p_exp->p_val;
    return p_exp->p_val;
}

Value *GenFunction::ast2ir_exp_gen(p_ast_exp p_exp)
{
    if (p_exp == nullptr)
        return nullptr;
    Value *p_instr = nullptr;
    switch (p_exp->kind)
    {
    case ast_exp::ast_exp_num: // 若是常量 直接返回该常量对应的操作数
        return ast2ir_exp_num_gen(p_exp);
    case ast_exp::ast_exp_ptr:
        return get_addr(p_exp->p_var, NULL, 0);
    case ast_exp::ast_exp_gep:
        p_instr = ast2ir_exp_gep_gen(p_exp);
        break;
    case ast_exp::ast_exp_load:
        p_instr = ast2ir_exp_load_gen(p_exp);
        break;
    case ast_exp::ast_exp_relational:
        p_instr = ast2ir_exp_relational_gen(p_exp);
        break;
    case ast_exp::ast_exp_binary:
        p_instr = ast2ir_exp_binary_gen(p_exp);
        break;
    case ast_exp::ast_exp_unary:
        p_instr = ast2ir_exp_unary_gen(p_exp);
        break;
    case ast_exp::ast_exp_logic:
        p_instr = ast2ir_exp_logic_gen(p_exp);
        break;
    case ast_exp::ast_exp_ulogic:
        p_instr = ast2ir_exp_ulogic_gen(p_exp);
        break;
    case ast_exp::ast_exp_call:
        p_instr = ast2ir_exp_call_gen(p_exp);
        break;
    case ast_exp::ast_exp_use:
        p_instr = ast2ir_exp_use_gen(p_exp);
        break;
    default:
        assert(0);
        break;
    }
    return p_instr;
}

void GenFunction::ast2ir_stmt_gen(BasicBlock *p_start, BasicBlock *p_next, p_ast_stmt p_stmt)
{
    if (!p_stmt)
        return;
    switch (p_stmt->type)
    {
    case ast_stmt::ast_stmt_assign:
        ast2ir_stmt_assign_gen(p_stmt->array.p_lval, p_stmt->array.p_rval, p_stmt->array.is_stack);
        break;
    case ast_stmt::ast_stmt_return:
        ast2ir_stmt_return_gen(p_stmt->branch.p_exp);
        break;
    case ast_stmt::ast_stmt_exp:
        ast2ir_stmt_exp_gen(p_stmt->branch.p_exp);
        break;
    case ast_stmt::ast_stmt_block:
        ast2ir_stmt_block_gen(p_start, p_next, p_stmt->p_block);
        break;
    case ast_stmt::ast_stmt_if:
        ast2ir_stmt_if_gen(p_start, p_next, p_stmt->branch.p_exp, p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt::ast_stmt_if_else:
        ast2ir_stmt_if_else_gen(p_start, p_next, p_stmt->branch.p_exp, p_stmt->branch.p_stmt_1, p_stmt->branch.p_stmt_2);
        break;
    case ast_stmt::ast_stmt_while:
        ast2ir_stmt_while_gen(p_stmt->branch.p_exp, p_stmt->branch.p_stmt_1);
        break;
    case ast_stmt::ast_stmt_break:
        ast2ir_stmt_break_gen(p_next);
        break;
    case ast_stmt::ast_stmt_continue:
        ast2ir_stmt_continue_gen(p_start);
        break;
    default:
        assert(0);
    }
}

void GenFunction::ast2ir_stmt_break_gen(BasicBlock *p_while_end_next)
{
    curBB->Set_jmp(p_while_end_next);
    BasicBlock *p_next = func->block_addnewBB();
    set_curBB(p_next);
}

void GenFunction::ast2ir_stmt_continue_gen(BasicBlock *p_while_cond)
{
    curBB->Set_jmp(p_while_cond);
    BasicBlock *p_next = func->block_addnewBB();
    set_curBB(p_next);
}

void GenFunction::ast2ir_stmt_assign_gen(p_ast_exp p_lval, p_ast_exp p_rval, bool is_stack)
{
    Value *p_src = ast2ir_exp_gen(p_rval);
    Value *p_addr = ast2ir_exp_gen(p_lval);
    new Store(p_addr, p_src, is_stack, curBB);
}

void GenFunction::ast2ir_stmt_return_gen(p_ast_exp p_exp)
{
    if (p_exp)
    {
        assert(p_ret);
        Value *ret_exp = ast2ir_exp_gen(p_exp);
        new Store(p_ret, ret_exp, true, curBB);
    }
    else
        assert(func->get_type()->get_type() == TypeEnum::Void);
    curBB->Set_jmp(retBB);
    this->new_curBB();
}

void GenFunction::ast2ir_stmt_exp_gen(p_ast_exp p_exp)
{
    if (p_exp == nullptr)
        return;
    ast2ir_exp_gen(p_exp);
}

void GenFunction::ast2ir_stmt_block_gen(BasicBlock *while_start, BasicBlock *while_end, p_ast_block p_block)
{
    p_list_head p_node;
    list_for_each(p_node, &p_block->stmt)
    {
        p_ast_stmt p_stmt = list_entry(p_node, ast_stmt, node);
        ast2ir_stmt_gen(while_start, while_end, p_stmt);
    }
}

void GenFunction::ast2ir_exp_cond_gen(BasicBlock *p_trueblock, BasicBlock *p_falseblock, p_ast_exp p_exp)
{
    assert(p_exp);
    if (p_exp->kind == ast_exp::ast_exp_logic && p_exp->l.l_op == ast_exp_op_bool_or)
    {
        BasicBlock *p_new_false = func->block_addnewBB();
        ast2ir_exp_cond_gen(p_trueblock, p_new_false, p_exp->l.p_bool_1);
        set_curBB(p_new_false);
        ast2ir_exp_cond_gen(p_trueblock, p_falseblock, p_exp->l.p_bool_2);
    }
    else if (p_exp->kind == ast_exp::ast_exp_logic && p_exp->l.l_op == ast_exp_op_bool_and)
    {
        BasicBlock *p_new_true = func->block_addnewBB();
        ast2ir_exp_cond_gen(p_new_true, p_falseblock, p_exp->l.p_bool_1);
        set_curBB(p_new_true);
        ast2ir_exp_cond_gen(p_trueblock, p_falseblock, p_exp->l.p_bool_2);
    }
    else if (p_exp->kind == ast_exp::ast_exp_ulogic && p_exp->ul.ul_op == ast_exp_op_bool_not)
    {
        ast2ir_exp_cond_gen(p_falseblock, p_trueblock, p_exp->ul.p_bool);
    }
    else
    {
        Value *p_cond = ast2ir_exp_gen(p_exp);
        curBB->Set_branch(p_cond, p_trueblock, p_falseblock);
    }
}

void GenFunction::ast2ir_stmt_if_gen(BasicBlock *p_cond, BasicBlock *p_next, p_ast_exp p_exp, p_ast_stmt p_stmt)
{
    BasicBlock *p_trueblock = func->block_addnewBB();
    BasicBlock *p_falseblock = func->block_addnewBB();

    ast2ir_exp_cond_gen(p_trueblock, p_falseblock, p_exp);
    set_curBB(p_trueblock);
    ast2ir_stmt_gen(p_cond, p_next, p_stmt);

    curBB->Set_jmp(p_falseblock);
    set_curBB(p_falseblock);
}

void GenFunction::ast2ir_stmt_if_else_gen(BasicBlock *p_while_cond, BasicBlock *p_while_end_next, p_ast_exp p_exp, p_ast_stmt p_stmt1, p_ast_stmt p_stmt2)
{
    BasicBlock *p_trueblock = func->block_addnewBB();
    BasicBlock *p_falseblock = func->block_addnewBB();
    BasicBlock *p_nextblock = func->block_addnewBB();

    ast2ir_exp_cond_gen(p_trueblock, p_falseblock, p_exp);

    set_curBB(p_trueblock);
    ast2ir_stmt_gen(p_while_cond, p_while_end_next, p_stmt1);
    curBB->Set_jmp(p_nextblock);

    set_curBB(p_falseblock);
    ast2ir_stmt_gen(p_while_cond, p_while_end_next, p_stmt2);
    curBB->Set_jmp(p_nextblock);

    set_curBB(p_nextblock);
}

void GenFunction::ast2ir_stmt_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt1)
{
    BasicBlock *p_trueblock = func->block_addnewBB();
    BasicBlock *p_nextblock = func->block_addnewBB();
    BasicBlock *p_condblock = func->block_addnewBB();

    ast2ir_exp_cond_gen(p_trueblock, p_nextblock, p_exp);

    set_curBB(p_trueblock);
    ast2ir_stmt_gen(p_condblock, p_nextblock, p_stmt1);
    curBB->Set_jmp(p_condblock);

    set_curBB(p_condblock);
    ast2ir_exp_cond_gen(p_trueblock, p_nextblock, p_exp);

    set_curBB(p_nextblock);
}